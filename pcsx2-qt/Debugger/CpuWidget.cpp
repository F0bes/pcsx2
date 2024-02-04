// SPDX-FileCopyrightText: 2002-2023 PCSX2 Dev Team
// SPDX-License-Identifier: LGPL-3.0+

#include "CpuWidget.h"

#include "DisassemblyWidget.h"
#include "BreakpointDialog.h"
#include "Models/BreakpointModel.h"
#include "Models/ThreadModel.h"
#include "Models/SavedAddressesModel.h"
#include "Debugger/DebuggerSettingsManager.h"

#include "DebugTools/DebugInterface.h"
#include "DebugTools/Breakpoints.h"
#include "DebugTools/MipsStackWalk.h"

#include "QtUtils.h"

#include "common/Console.h"

#include <QtGui/QClipboard>
#include <QtWidgets/QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <QtCore/QFutureWatcher>
#include <QtCore/QRegularExpression>
#include <QtCore/QRegularExpressionMatchIterator>
#include <QtCore/QStringList>
#include <QtWidgets/QScrollBar>

using namespace QtUtils;
using namespace MipsStackWalk;

using SearchComparison = CpuWidget::SearchComparison;
using SearchType = CpuWidget::SearchType;

CpuWidget::CpuWidget(QWidget* parent, DebugInterface& cpu)
	: m_cpu(cpu)
	, m_bpModel(cpu)
	, m_threadModel(cpu)
	, m_stackModel(cpu)
	, m_savedAddressesModel(cpu)
{
	m_ui.setupUi(this);

	connect(g_emu_thread, &EmuThread::onVMPaused, this, &CpuWidget::onVMPaused);
	connect(g_emu_thread, &EmuThread::onGameChanged, [this](const QString& title) {
		if (title.isEmpty())
			return;
		// Don't overwrite users BPs/Saved Addresses unless they have a clean state.
		if (m_bpModel.rowCount() == 0)
			DebuggerSettingsManager::loadGameSettings(&m_bpModel);
		if (m_savedAddressesModel.rowCount() == 0)
			DebuggerSettingsManager::loadGameSettings(&m_savedAddressesModel);
	});

	connect(m_ui.registerWidget, &RegisterWidget::gotoInDisasm, m_ui.disassemblyWidget, &DisassemblyWidget::gotoAddress);
	connect(m_ui.memoryviewWidget, &MemoryViewWidget::gotoInDisasm, m_ui.disassemblyWidget, &DisassemblyWidget::gotoAddress);
	connect(m_ui.memoryviewWidget, &MemoryViewWidget::addToSavedAddresses, this, &CpuWidget::addAddressToSavedAddressesList);

	connect(m_ui.registerWidget, &RegisterWidget::gotoInMemory, m_ui.memoryviewWidget, &MemoryViewWidget::gotoAddress);
	connect(m_ui.disassemblyWidget, &DisassemblyWidget::gotoInMemory, m_ui.memoryviewWidget, &MemoryViewWidget::gotoAddress);

	connect(m_ui.memoryviewWidget, &MemoryViewWidget::VMUpdate, this, &CpuWidget::reloadCPUWidgets);
	connect(m_ui.registerWidget, &RegisterWidget::VMUpdate, this, &CpuWidget::reloadCPUWidgets);
	connect(m_ui.disassemblyWidget, &DisassemblyWidget::VMUpdate, this, &CpuWidget::reloadCPUWidgets);

	connect(m_ui.breakpointList, &QTableView::customContextMenuRequested, this, &CpuWidget::onBPListContextMenu);
	connect(m_ui.breakpointList, &QTableView::doubleClicked, this, &CpuWidget::onBPListDoubleClicked);

	m_ui.breakpointList->setModel(&m_bpModel);
	for (std::size_t i = 0; auto mode : BreakpointModel::HeaderResizeModes)
	{
		m_ui.breakpointList->horizontalHeader()->setSectionResizeMode(i, mode);
		i++;
	}

	connect(m_ui.threadList, &QTableView::customContextMenuRequested, this, &CpuWidget::onThreadListContextMenu);
	connect(m_ui.threadList, &QTableView::doubleClicked, this, &CpuWidget::onThreadListDoubleClick);

	m_threadProxyModel.setSourceModel(&m_threadModel);
	m_threadProxyModel.setSortRole(Qt::UserRole);
	m_ui.threadList->setModel(&m_threadProxyModel);
	m_ui.threadList->setSortingEnabled(true);
	m_ui.threadList->sortByColumn(ThreadModel::ThreadColumns::ID, Qt::SortOrder::AscendingOrder);
	for (std::size_t i = 0; auto mode : ThreadModel::HeaderResizeModes)
	{
		m_ui.threadList->horizontalHeader()->setSectionResizeMode(i, mode);
		i++;
	}

	connect(m_ui.stackList, &QTableView::customContextMenuRequested, this, &CpuWidget::onStackListContextMenu);
	connect(m_ui.stackList, &QTableView::doubleClicked, this, &CpuWidget::onStackListDoubleClick);

	m_ui.stackList->setModel(&m_stackModel);
	for (std::size_t i = 0; auto mode : StackModel::HeaderResizeModes)
	{
		m_ui.stackList->horizontalHeader()->setSectionResizeMode(i, mode);
		i++;
	}

	m_ui.listSearchResults->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_ui.btnSearch, &QPushButton::clicked, this, &CpuWidget::onSearchButtonClicked);
	connect(m_ui.btnFilterSearch, &QPushButton::clicked, this, &CpuWidget::onSearchButtonClicked);
	connect(m_ui.listSearchResults, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item) {
		m_ui.tabWidget->setCurrentWidget(m_ui.tab_memory);
		m_ui.memoryviewWidget->gotoAddress(item->text().toUInt(nullptr, 16));
	});
	connect(m_ui.listSearchResults->verticalScrollBar(), &QScrollBar::valueChanged, this, &CpuWidget::onSearchResultsListScroll);
	connect(m_ui.listSearchResults, &QListView::customContextMenuRequested, this, &CpuWidget::onListSearchResultsContextMenu);
	connect(m_ui.cmbSearchType, &QComboBox::currentIndexChanged, [this](int i) {
		if (i < 4)
			m_ui.chkSearchHex->setEnabled(true);
		else
			m_ui.chkSearchHex->setEnabled(false);
	});
	m_ui.disassemblyWidget->SetCpu(&cpu);
	m_ui.registerWidget->SetCpu(&cpu);
	m_ui.memoryviewWidget->SetCpu(&cpu);

	this->repaint();

	// Ensures we don't retrigger the load results function unintentionally
	m_resultsLoadTimer.setInterval(100);
	m_resultsLoadTimer.setSingleShot(true);
	connect(&m_resultsLoadTimer, &QTimer::timeout, this, &CpuWidget::loadSearchResults);

	m_ui.savedAddressesList->setModel(&m_savedAddressesModel);
	m_ui.savedAddressesList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_ui.savedAddressesList, &QTableView::customContextMenuRequested, this, &CpuWidget::onSavedAddressesListContextMenu);
	for (std::size_t i = 0; auto mode : SavedAddressesModel::HeaderResizeModes)
	{
		m_ui.savedAddressesList->horizontalHeader()->setSectionResizeMode(i++, mode);
	}
	QTableView* savedAddressesTableView = m_ui.savedAddressesList;
	connect(m_ui.savedAddressesList->model(), &QAbstractItemModel::dataChanged, [savedAddressesTableView](const QModelIndex& topLeft) {
		savedAddressesTableView->resizeColumnToContents(topLeft.column());
	});

	setupSymbolTrees();

	DebuggerSettingsManager::loadGameSettings(&m_bpModel);
	DebuggerSettingsManager::loadGameSettings(&m_savedAddressesModel);
}

CpuWidget::~CpuWidget() = default;

void CpuWidget::setupSymbolTrees()
{
	m_ui.tabFunctions->setLayout(new QVBoxLayout());
	m_ui.tabGlobalVariables->setLayout(new QVBoxLayout());
	m_ui.tabLocalVariables->setLayout(new QVBoxLayout());
	
	m_ui.tabFunctions->layout()->setContentsMargins(0, 0, 0, 0);
	m_ui.tabGlobalVariables->layout()->setContentsMargins(0, 0, 0, 0);
	m_ui.tabLocalVariables->layout()->setContentsMargins(0, 0, 0, 0);
	
	m_function_tree = new FunctionTreeWidget(m_cpu);
	m_global_variable_tree = new GlobalVariableTreeWidget(m_cpu);
	m_local_variable_tree = new LocalVariableTreeWidget(m_cpu);

	m_ui.tabFunctions->layout()->addWidget(m_function_tree);
	m_ui.tabGlobalVariables->layout()->addWidget(m_global_variable_tree);
	m_ui.tabLocalVariables->layout()->addWidget(m_local_variable_tree);

	connect(m_ui.tabWidgetRegFunc, &QTabWidget::currentChanged, m_function_tree, &SymbolTreeWidget::update);
	connect(m_ui.tabWidget, &QTabWidget::currentChanged, m_global_variable_tree, &SymbolTreeWidget::update);
	connect(m_ui.tabWidget, &QTabWidget::currentChanged, m_local_variable_tree, &SymbolTreeWidget::update);
}

void CpuWidget::paintEvent(QPaintEvent* event)
{
	m_ui.registerWidget->update();
	m_ui.disassemblyWidget->update();
	m_ui.memoryviewWidget->update();
}

// The cpu shouldn't be alive when these are called
// But make sure it isn't just in case
void CpuWidget::onStepInto()
{
	if (!m_cpu.isAlive() || !m_cpu.isCpuPaused())
		return;

	// Allow the cpu to skip this pc if it is a breakpoint
	CBreakPoints::SetSkipFirst(m_cpu.getCpuType(), m_cpu.getPC());

	const u32 pc = m_cpu.getPC();
	const MIPSAnalyst::MipsOpcodeInfo info = MIPSAnalyst::GetOpcodeInfo(&m_cpu, pc);

	u32 bpAddr = pc + 0x4; // Default to the next instruction

	if (info.isBranch)
	{
		if (!info.isConditional)
		{
			bpAddr = info.branchTarget;
		}
		else
		{
			if (info.conditionMet)
			{
				bpAddr = info.branchTarget;
			}
			else
			{
				bpAddr = pc + (2 * 4); // Skip branch delay slot
			}
		}
	}

	if (info.isSyscall)
		bpAddr = info.branchTarget; // Syscalls are always taken

	Host::RunOnCPUThread([&] {
		CBreakPoints::AddBreakPoint(m_cpu.getCpuType(), bpAddr, true);
		m_cpu.resumeCpu();
	});

	this->repaint();
}

void CpuWidget::onStepOut()
{
	if (!m_cpu.isAlive() || !m_cpu.isCpuPaused())
		return;

	// Allow the cpu to skip this pc if it is a breakpoint
	CBreakPoints::SetSkipFirst(m_cpu.getCpuType(), m_cpu.getPC());

	if (m_stackModel.rowCount() < 2)
		return;

	Host::RunOnCPUThread([&] {
		CBreakPoints::AddBreakPoint(m_cpu.getCpuType(), m_stackModel.data(m_stackModel.index(1, StackModel::PC), Qt::UserRole).toUInt(), true);
		m_cpu.resumeCpu();
	});

	this->repaint();
}

void CpuWidget::onStepOver()
{
	if (!m_cpu.isAlive() || !m_cpu.isCpuPaused())
		return;

	const u32 pc = m_cpu.getPC();
	const MIPSAnalyst::MipsOpcodeInfo info = MIPSAnalyst::GetOpcodeInfo(&m_cpu, pc);

	u32 bpAddr = pc + 0x4; // Default to the next instruction

	if (info.isBranch)
	{
		if (!info.isConditional)
		{
			if (info.isLinkedBranch) // jal, jalr
			{
				// it's a function call with a delay slot - skip that too
				bpAddr += 4;
			}
			else // j, ...
			{
				// in case of absolute branches, set the breakpoint at the branch target
				bpAddr = info.branchTarget;
			}
		}
		else // beq, ...
		{
			if (info.conditionMet)
			{
				bpAddr = info.branchTarget;
			}
			else
			{
				bpAddr = pc + (2 * 4); // Skip branch delay slot
			}
		}
	}

	Host::RunOnCPUThread([&] {
		CBreakPoints::AddBreakPoint(m_cpu.getCpuType(), bpAddr, true);
		m_cpu.resumeCpu();
	});

	this->repaint();
}

void CpuWidget::onVMPaused()
{
	// Stops us from telling the disassembly dialog to jump somwhere because breakpoint code paused the core.
	if (CBreakPoints::GetCorePaused())
	{
		CBreakPoints::SetCorePaused(false);
	}
	else
	{
		m_ui.disassemblyWidget->gotoAddress(m_cpu.getPC(), false);
	}

	reloadCPUWidgets();
	this->repaint();
}

void CpuWidget::updateBreakpoints()
{
	m_bpModel.refreshData();
}

void CpuWidget::onBPListDoubleClicked(const QModelIndex& index)
{
	if (index.isValid())
	{
		if (index.column() == BreakpointModel::OFFSET)
		{
			m_ui.disassemblyWidget->gotoAddress(m_bpModel.data(index, BreakpointModel::DataRole).toUInt());
		}
	}
}

void CpuWidget::onBPListContextMenu(QPoint pos)
{
	QMenu* contextMenu = new QMenu(tr("Breakpoint List Context Menu"), m_ui.breakpointList);
	if (m_cpu.isAlive())
	{

		QAction* newAction = new QAction(tr("New"), m_ui.breakpointList);
		connect(newAction, &QAction::triggered, this, &CpuWidget::contextBPListNew);
		contextMenu->addAction(newAction);

		const QItemSelectionModel* selModel = m_ui.breakpointList->selectionModel();

		if (selModel->hasSelection())
		{
			QAction* editAction = new QAction(tr("Edit"), m_ui.breakpointList);
			connect(editAction, &QAction::triggered, this, &CpuWidget::contextBPListEdit);
			contextMenu->addAction(editAction);

			if (selModel->selectedIndexes().count() == 1)
			{
				QAction* copyAction = new QAction(tr("Copy"), m_ui.breakpointList);
				connect(copyAction, &QAction::triggered, this, &CpuWidget::contextBPListCopy);
				contextMenu->addAction(copyAction);
			}

			QAction* deleteAction = new QAction(tr("Delete"), m_ui.breakpointList);
			connect(deleteAction, &QAction::triggered, this, &CpuWidget::contextBPListDelete);
			contextMenu->addAction(deleteAction);
		}
	}

	contextMenu->addSeparator();
	if (m_bpModel.rowCount() > 0)
	{
		QAction* actionExport = new QAction(tr("Copy all as CSV"), m_ui.breakpointList);
		connect(actionExport, &QAction::triggered, [this]() {
			// It's important to use the Export Role here to allow pasting to be translation agnostic
			QGuiApplication::clipboard()->setText(QtUtils::AbstractItemModelToCSV(m_ui.breakpointList->model(), BreakpointModel::ExportRole, true));
		});
		contextMenu->addAction(actionExport);
	}

	if (m_cpu.isAlive())
	{
		QAction* actionImport = new QAction(tr("Paste from CSV"), m_ui.breakpointList);
		connect(actionImport, &QAction::triggered, this, &CpuWidget::contextBPListPasteCSV);
		contextMenu->addAction(actionImport);

		QAction* actionLoad = new QAction(tr("Load from Settings"), m_ui.breakpointList);
		connect(actionLoad, &QAction::triggered, [this]() {
			m_bpModel.clear();
			DebuggerSettingsManager::loadGameSettings(&m_bpModel);
		});
		contextMenu->addAction(actionLoad);

		QAction* actionSave = new QAction(tr("Save to Settings"), m_ui.breakpointList);
		connect(actionSave, &QAction::triggered, this, &CpuWidget::saveBreakpointsToDebuggerSettings);
		contextMenu->addAction(actionSave);
	}

	contextMenu->popup(m_ui.breakpointList->viewport()->mapToGlobal(pos));
}

void CpuWidget::contextBPListCopy()
{
	const QItemSelectionModel* selModel = m_ui.breakpointList->selectionModel();

	if (!selModel->hasSelection())
		return;

	QGuiApplication::clipboard()->setText(m_bpModel.data(selModel->currentIndex()).toString());
}

void CpuWidget::contextBPListDelete()
{
	const QItemSelectionModel* selModel = m_ui.breakpointList->selectionModel();

	if (!selModel->hasSelection())
		return;

	QModelIndexList rows = selModel->selectedIndexes();

	std::sort(rows.begin(), rows.end(), [](const QModelIndex& a, const QModelIndex& b) {
		return a.row() > b.row();
	});

	for (const QModelIndex& index : rows)
	{
		m_bpModel.removeRows(index.row(), 1);
	}
}

void CpuWidget::contextBPListNew()
{
	BreakpointDialog* bpDialog = new BreakpointDialog(this, &m_cpu, m_bpModel);
	bpDialog->show();
}

void CpuWidget::contextBPListEdit()
{
	const QItemSelectionModel* selModel = m_ui.breakpointList->selectionModel();

	if (!selModel->hasSelection())
		return;

	const int selectedRow = selModel->selectedIndexes().first().row();

	auto bpObject = m_bpModel.at(selectedRow);

	BreakpointDialog* bpDialog = new BreakpointDialog(this, &m_cpu, m_bpModel, bpObject, selectedRow);
	bpDialog->show();
}

void CpuWidget::contextBPListPasteCSV()
{
	QString csv = QGuiApplication::clipboard()->text();
	// Skip header
	csv = csv.mid(csv.indexOf('\n') + 1);

	for (const QString& line : csv.split('\n'))
	{
		QStringList fields;
		// In order to handle text with commas in them we must wrap values in quotes to mark
		// where a value starts and end so that text commas aren't identified as delimiters.
		// So matches each quote pair, parse it out, and removes the quotes to get the value.
		QRegularExpression eachQuotePair(R"("([^"]|\\.)*")");
		QRegularExpressionMatchIterator it = eachQuotePair.globalMatch(line);
		while (it.hasNext())
		{
			QRegularExpressionMatch match = it.next();
			QString matchedValue = match.captured(0);
			fields << matchedValue.mid(1, matchedValue.length() - 2);
		}
		m_bpModel.loadBreakpointFromFieldList(fields);
	}
}

void CpuWidget::onSavedAddressesListContextMenu(QPoint pos)
{
	QMenu* contextMenu = new QMenu("Saved Addresses List Context Menu", m_ui.savedAddressesList);

	QAction* newAction = new QAction(tr("New"), m_ui.savedAddressesList);
	connect(newAction, &QAction::triggered, this, &CpuWidget::contextSavedAddressesListNew);
	contextMenu->addAction(newAction);

	const QModelIndex indexAtPos = m_ui.savedAddressesList->indexAt(pos);
	const bool isIndexValid = indexAtPos.isValid();

	if (isIndexValid)
	{
		if (m_cpu.isAlive())
		{
			QAction* goToAddressMemViewAction = new QAction(tr("Go to in Memory View"), m_ui.savedAddressesList);
			connect(goToAddressMemViewAction, &QAction::triggered, this, [this, indexAtPos]() {
				const QModelIndex rowAddressIndex = m_ui.savedAddressesList->model()->index(indexAtPos.row(), 0, QModelIndex());
				m_ui.memoryviewWidget->gotoAddress(m_ui.savedAddressesList->model()->data(rowAddressIndex, Qt::UserRole).toUInt());
				m_ui.tabWidget->setCurrentWidget(m_ui.tab_memory);
			});
			contextMenu->addAction(goToAddressMemViewAction);

			QAction* goToAddressDisassemblyAction = new QAction(tr("Go to in Disassembly"), m_ui.savedAddressesList);
			connect(goToAddressDisassemblyAction, &QAction::triggered, this, [this, indexAtPos]() {
				const QModelIndex rowAddressIndex = m_ui.savedAddressesList->model()->index(indexAtPos.row(), 0, QModelIndex());
				m_ui.disassemblyWidget->gotoAddress(m_ui.savedAddressesList->model()->data(rowAddressIndex, Qt::UserRole).toUInt());
			});
			contextMenu->addAction(goToAddressDisassemblyAction);
		}

		QAction* copyAction = new QAction(indexAtPos.column() == 0 ? tr("Copy Address") : tr("Copy Text"), m_ui.savedAddressesList);
		connect(copyAction, &QAction::triggered, [this, indexAtPos]() {
			QGuiApplication::clipboard()->setText(m_ui.savedAddressesList->model()->data(indexAtPos, Qt::DisplayRole).toString());
		});
		contextMenu->addAction(copyAction);
	}

	if (m_ui.savedAddressesList->model()->rowCount() > 0)
	{
		QAction* actionExportCSV = new QAction(tr("Copy all as CSV"), m_ui.savedAddressesList);
		connect(actionExportCSV, &QAction::triggered, [this]() {
			QGuiApplication::clipboard()->setText(QtUtils::AbstractItemModelToCSV(m_ui.savedAddressesList->model(), Qt::DisplayRole, true));
		});
		contextMenu->addAction(actionExportCSV);
	}

	QAction* actionImportCSV = new QAction(tr("Paste from CSV"), m_ui.savedAddressesList);
	connect(actionImportCSV, &QAction::triggered, this, &CpuWidget::contextSavedAddressesListPasteCSV);
	contextMenu->addAction(actionImportCSV);

	if (m_cpu.isAlive())
	{
		QAction* actionLoad = new QAction(tr("Load from Settings"), m_ui.savedAddressesList);
		connect(actionLoad, &QAction::triggered, [this]() {
			m_savedAddressesModel.clear();
			DebuggerSettingsManager::loadGameSettings(&m_savedAddressesModel);
		});
		contextMenu->addAction(actionLoad);

		QAction* actionSave = new QAction(tr("Save to Settings"), m_ui.savedAddressesList);
		connect(actionSave, &QAction::triggered, this, &CpuWidget::saveSavedAddressesToDebuggerSettings);
		contextMenu->addAction(actionSave);
	}

	if (isIndexValid)
	{
		QAction* deleteAction = new QAction(tr("Delete"), m_ui.savedAddressesList);
		connect(deleteAction, &QAction::triggered, this, [this, indexAtPos]() {
			m_ui.savedAddressesList->model()->removeRows(indexAtPos.row(), 1);
		});
		contextMenu->addAction(deleteAction);
	}

	contextMenu->popup(m_ui.savedAddressesList->viewport()->mapToGlobal(pos));
}

void CpuWidget::contextSavedAddressesListPasteCSV()
{
	QString csv = QGuiApplication::clipboard()->text();
	// Skip header
	csv = csv.mid(csv.indexOf('\n') + 1);

	for (const QString& line : csv.split('\n'))
	{
		QStringList fields;
		// In order to handle text with commas in them we must wrap values in quotes to mark
		// where a value starts and end so that text commas aren't identified as delimiters.
		// So matches each quote pair, parse it out, and removes the quotes to get the value.
		QRegularExpression eachQuotePair(R"("([^"]|\\.)*")");
		QRegularExpressionMatchIterator it = eachQuotePair.globalMatch(line);
		while (it.hasNext())
		{
			QRegularExpressionMatch match = it.next();
			QString matchedValue = match.captured(0);
			fields << matchedValue.mid(1, matchedValue.length() - 2);
		}

		m_savedAddressesModel.loadSavedAddressFromFieldList(fields);
	}
}

void CpuWidget::contextSavedAddressesListNew()
{
	qobject_cast<SavedAddressesModel*>(m_ui.savedAddressesList->model())->addRow();
	const u32 rowCount = m_ui.savedAddressesList->model()->rowCount();
	m_ui.savedAddressesList->edit(m_ui.savedAddressesList->model()->index(rowCount - 1, 0));
}

void CpuWidget::addAddressToSavedAddressesList(u32 address)
{
	qobject_cast<SavedAddressesModel*>(m_ui.savedAddressesList->model())->addRow();
	const u32 rowCount = m_ui.savedAddressesList->model()->rowCount();
	const QModelIndex addressIndex = m_ui.savedAddressesList->model()->index(rowCount - 1, 0);
	m_ui.tabWidget->setCurrentWidget(m_ui.tab_savedaddresses);
	m_ui.savedAddressesList->model()->setData(addressIndex, address, Qt::UserRole);
	m_ui.savedAddressesList->edit(m_ui.savedAddressesList->model()->index(rowCount - 1, 1));
}

void CpuWidget::contextSearchResultGoToDisassembly()
{
	const QItemSelectionModel* selModel = m_ui.listSearchResults->selectionModel();
	if (!selModel->hasSelection())
		return;

	m_ui.disassemblyWidget->gotoAddress(m_ui.listSearchResults->selectedItems().first()->data(Qt::UserRole).toUInt());
}

void CpuWidget::contextRemoveSearchResult()
{
	const QItemSelectionModel* selModel = m_ui.listSearchResults->selectionModel();
	if (!selModel->hasSelection())
		return;

	const int selectedResultIndex = m_ui.listSearchResults->row(m_ui.listSearchResults->selectedItems().first());
	const auto* rowToRemove = m_ui.listSearchResults->takeItem(selectedResultIndex);
	if (m_searchResults.size() > static_cast<size_t>(selectedResultIndex) && m_searchResults.at(selectedResultIndex) == rowToRemove->data(Qt::UserRole).toUInt())
	{
		m_searchResults.erase(m_searchResults.begin() + selectedResultIndex);
	}
	delete rowToRemove;
}

void CpuWidget::contextCopySearchResultAddress()
{
	if (!m_ui.listSearchResults->selectionModel()->hasSelection())
		return;

	const u32 selectedResultIndex = m_ui.listSearchResults->row(m_ui.listSearchResults->selectedItems().first());
	const u32 rowAddress = m_ui.listSearchResults->item(selectedResultIndex)->data(Qt::UserRole).toUInt();
	const QString addressString = FilledQStringFromValue(rowAddress, 16);
	QApplication::clipboard()->setText(addressString);
}

void CpuWidget::updateThreads()
{
	m_threadModel.refreshData();
}

void CpuWidget::onThreadListContextMenu(QPoint pos)
{
	if (!m_ui.threadList->selectionModel()->hasSelection())
		return;

	QMenu* contextMenu = new QMenu(tr("Thread List Context Menu"), m_ui.threadList);

	QAction* actionCopy = new QAction(tr("Copy"), m_ui.threadList);
	connect(actionCopy, &QAction::triggered, [this]() {
		const auto* selModel = m_ui.threadList->selectionModel();

		if (!selModel->hasSelection())
			return;

		QGuiApplication::clipboard()->setText(m_ui.threadList->model()->data(selModel->currentIndex()).toString());
	});
	contextMenu->addAction(actionCopy);

	contextMenu->addSeparator();

	QAction* actionExport = new QAction(tr("Copy all as CSV"), m_ui.threadList);
	connect(actionExport, &QAction::triggered, [this]() {
		QGuiApplication::clipboard()->setText(QtUtils::AbstractItemModelToCSV(m_ui.threadList->model()));
	});
	contextMenu->addAction(actionExport);

	contextMenu->popup(m_ui.threadList->viewport()->mapToGlobal(pos));
}

void CpuWidget::onThreadListDoubleClick(const QModelIndex& index)
{
	switch (index.column())
	{
		case ThreadModel::ThreadColumns::ENTRY:
			m_ui.memoryviewWidget->gotoAddress(m_ui.threadList->model()->data(index, Qt::UserRole).toUInt());
			m_ui.tabWidget->setCurrentWidget(m_ui.tab_memory);
			break;
		default: // Default to PC
			m_ui.disassemblyWidget->gotoAddress(m_ui.threadList->model()->data(m_ui.threadList->model()->index(index.row(), ThreadModel::ThreadColumns::PC), Qt::UserRole).toUInt());
			break;
	}
}

void CpuWidget::updateStackFrames()
{
	m_stackModel.refreshData();
}

void CpuWidget::onListSearchResultsContextMenu(QPoint pos)
{
	QMenu* contextMenu = new QMenu(tr("Search Results List Context Menu"), m_ui.listSearchResults);
	const QItemSelectionModel* selModel = m_ui.listSearchResults->selectionModel();
	const auto listSearchResults = m_ui.listSearchResults;

	if (selModel->hasSelection())
	{
		QAction* copyAddressAction = new QAction(tr("Copy Address"), m_ui.listSearchResults);
		connect(copyAddressAction, &QAction::triggered, this, &CpuWidget::contextCopySearchResultAddress);
		contextMenu->addAction(copyAddressAction);

		QAction* goToDisassemblyAction = new QAction(tr("Go to in Disassembly"), m_ui.listSearchResults);
		connect(goToDisassemblyAction, &QAction::triggered, this, &CpuWidget::contextSearchResultGoToDisassembly);
		contextMenu->addAction(goToDisassemblyAction);

		QAction* addToSavedAddressesAction = new QAction(tr("Add to Saved Memory Addresses"), m_ui.listSearchResults);
		connect(addToSavedAddressesAction, &QAction::triggered, this, [this, listSearchResults]() {
			addAddressToSavedAddressesList(listSearchResults->selectedItems().first()->data(Qt::UserRole).toUInt());
		});
		contextMenu->addAction(addToSavedAddressesAction);

		QAction* removeResultAction = new QAction(tr("Remove Result"), m_ui.listSearchResults);
		connect(removeResultAction, &QAction::triggered, this, &CpuWidget::contextRemoveSearchResult);
		contextMenu->addAction(removeResultAction);
	}

	contextMenu->popup(m_ui.listSearchResults->viewport()->mapToGlobal(pos));
}

void CpuWidget::onStackListContextMenu(QPoint pos)
{
	if (!m_ui.stackList->selectionModel()->hasSelection())
		return;

	QMenu* contextMenu = new QMenu(tr("Stack List Context Menu"), m_ui.stackList);

	QAction* actionCopy = new QAction(tr("Copy"), m_ui.stackList);
	connect(actionCopy, &QAction::triggered, [this]() {
		const auto* selModel = m_ui.stackList->selectionModel();

		if (!selModel->hasSelection())
			return;

		QGuiApplication::clipboard()->setText(m_ui.stackList->model()->data(selModel->currentIndex()).toString());
	});
	contextMenu->addAction(actionCopy);

	contextMenu->addSeparator();

	QAction* actionExport = new QAction(tr("Copy all as CSV"), m_ui.stackList);
	connect(actionExport, &QAction::triggered, [this]() {
		QGuiApplication::clipboard()->setText(QtUtils::AbstractItemModelToCSV(m_ui.stackList->model()));
	});
	contextMenu->addAction(actionExport);

	contextMenu->popup(m_ui.stackList->viewport()->mapToGlobal(pos));
}

void CpuWidget::onStackListDoubleClick(const QModelIndex& index)
{
	switch (index.column())
	{
		case StackModel::StackModel::ENTRY:
		case StackModel::StackModel::ENTRY_LABEL:
			m_ui.disassemblyWidget->gotoAddress(m_ui.stackList->model()->data(m_ui.stackList->model()->index(index.row(), StackModel::StackColumns::ENTRY), Qt::UserRole).toUInt());
			break;
		case StackModel::StackModel::SP:
			m_ui.memoryviewWidget->gotoAddress(m_ui.stackList->model()->data(index, Qt::UserRole).toUInt());
			m_ui.tabWidget->setCurrentWidget(m_ui.tab_memory);
			break;
		default: // Default to PC
			m_ui.disassemblyWidget->gotoAddress(m_ui.stackList->model()->data(m_ui.stackList->model()->index(index.row(), StackModel::StackColumns::PC), Qt::UserRole).toUInt());
			break;
	}
}

template <typename T>
static T readValueAtAddress(DebugInterface* cpu, u32 addr)
{
	T val = 0;
	switch (sizeof(T))
	{
		case sizeof(u8):
			val = cpu->read8(addr);
			break;
		case sizeof(u16):
			val = cpu->read16(addr);
			break;
		case sizeof(u32):
		{
			val = cpu->read32(addr);
			break;
		}
		case sizeof(u64):
		{
			val = cpu->read64(addr);
			break;
		}
	}
	return val;
}

template <typename T>
static bool memoryValueComparator(SearchComparison searchComparison, T searchValue, T readValue)
{
	const bool isNotOperator = searchComparison == SearchComparison::NotEquals;
	switch (searchComparison)
	{
		case SearchComparison::Equals:
		case SearchComparison::NotEquals:
		{
			bool areValuesEqual = false;
			if constexpr (std::is_same_v<T, float>)
			{
				const T fTop = searchValue + 0.00001f;
				const T fBottom = searchValue - 0.00001f;
				const T memValue = std::bit_cast<float, u32>(readValue);
				areValuesEqual = (fBottom < memValue && memValue < fTop);
			}
			else if constexpr (std::is_same_v<T, double>)
			{
				const double dTop = searchValue + 0.00001f;
				const double dBottom = searchValue - 0.00001f;
				const double memValue = std::bit_cast<double, u64>(readValue);
				areValuesEqual = (dBottom < memValue && memValue < dTop);
			}
			else
			{
				areValuesEqual = searchValue == readValue;
			}
			return isNotOperator ? !areValuesEqual : areValuesEqual;
			break;
		}
		case SearchComparison::GreaterThan:
		case SearchComparison::GreaterThanOrEqual:
		case SearchComparison::LessThan:
		case SearchComparison::LessThanOrEqual:
		{
			const bool hasEqualsCheck = searchComparison == SearchComparison::GreaterThanOrEqual || searchComparison == SearchComparison::LessThanOrEqual;
			if (hasEqualsCheck && memoryValueComparator(SearchComparison::Equals, searchValue, readValue))
				return true;

			const bool isGreaterOperator = searchComparison == SearchComparison::GreaterThan || searchComparison == SearchComparison::GreaterThanOrEqual;
			if (std::is_same_v<T, float>)
			{
				const T fTop = searchValue + 0.00001f;
				const T fBottom = searchValue - 0.00001f;
				const T memValue = std::bit_cast<float, u32>(readValue);
				const bool isGreater = memValue > fTop;
				const bool isLesser = memValue < fBottom;
				return isGreaterOperator ? isGreater : isLesser;
			}
			else if (std::is_same_v<T, double>)
			{
				const double dTop = searchValue + 0.00001f;
				const double dBottom = searchValue - 0.00001f;
				const double memValue = std::bit_cast<double, u64>(readValue);
				const bool isGreater = memValue > dTop;
				const bool isLesser = memValue < dBottom;
				return isGreaterOperator ? isGreater : isLesser;
			}

			return isGreaterOperator ? (readValue > searchValue) : (readValue < searchValue);
		}
		default:
			Console.Error("Debugger: Unknown type when doing memory search!");
			return false;
	}
}

template <typename T>
std::vector<u32> searchWorker(DebugInterface* cpu, std::vector<u32> searchAddresses, SearchComparison searchComparison, u32 start, u32 end, T searchValue)
{
	std::vector<u32> hitAddresses;
	const bool isSearchingRange = searchAddresses.size() <= 0;
	if (isSearchingRange)
	{
		for (u32 addr = start; addr < end; addr += sizeof(T))
		{
			if (!cpu->isValidAddress(addr))
				continue;
			T readValue = readValueAtAddress<T>(cpu, addr);
			if (memoryValueComparator(searchComparison, searchValue, readValue))
			{
				hitAddresses.push_back(addr);
			}
		}
	}
	else
	{
		for (const u32 addr : searchAddresses)
		{
			if (!cpu->isValidAddress(addr))
				continue;
			T readValue = readValueAtAddress<T>(cpu, addr);
			if (memoryValueComparator(searchComparison, searchValue, readValue))
			{
				hitAddresses.push_back(addr);
			}
		}
	}
	return hitAddresses;
}

static bool compareByteArrayAtAddress(DebugInterface* cpu, SearchComparison searchComparison, u32 addr, QByteArray value)
{
	const bool isNotOperator = searchComparison == SearchComparison::NotEquals;
	for (qsizetype i = 0; i < value.length(); i++)
	{
		const char nextByte = cpu->read8(addr + i);
		switch (searchComparison)
		{
			case SearchComparison::Equals:
			{
				if (nextByte != value[i])
					return false;
				break;
			}
			case SearchComparison::NotEquals:
			{
				if (nextByte != value[i])
					return true;
				break;
			}
			default:
			{
				Console.Error("Debugger: Unknown search comparison when doing memory search");
				return false;
			}
		}
	}
	return !isNotOperator;
}

static std::vector<u32> searchWorkerByteArray(DebugInterface* cpu, SearchComparison searchComparison, std::vector<u32> searchAddresses, u32 start, u32 end, QByteArray value)
{
	std::vector<u32> hitAddresses;
	const bool isSearchingRange = searchAddresses.size() <= 0;
	if (isSearchingRange)
	{
		for (u32 addr = start; addr < end; addr += 1)
		{
			if (compareByteArrayAtAddress(cpu, searchComparison, addr, value))
			{
				hitAddresses.emplace_back(addr);
				addr += value.length() - 1;
			}
		}
	}
	else
	{
		for (u32 addr : searchAddresses)
		{
			if (compareByteArrayAtAddress(cpu, searchComparison, addr, value))
			{
				hitAddresses.emplace_back(addr);
			}
		}
	}
	return hitAddresses;
}

std::vector<u32> startWorker(DebugInterface* cpu, const SearchType type, const SearchComparison searchComparison, std::vector<u32> searchAddresses, u32 start, u32 end, QString value, int base)
{
	const bool isSigned = value.startsWith("-");
	switch (type)
	{
		case SearchType::ByteType:
			return isSigned ? searchWorker<s8>(cpu, searchAddresses, searchComparison, start, end, value.toShort(nullptr, base)) : searchWorker<u8>(cpu, searchAddresses, searchComparison, start, end, value.toUShort(nullptr, base));
		case SearchType::Int16Type:
			return isSigned ? searchWorker<s16>(cpu, searchAddresses, searchComparison, start, end, value.toShort(nullptr, base)) : searchWorker<u16>(cpu, searchAddresses, searchComparison, start, end, value.toUShort(nullptr, base));
		case SearchType::Int32Type:
			return isSigned ? searchWorker<s32>(cpu, searchAddresses, searchComparison, start, end, value.toInt(nullptr, base)) : searchWorker<u32>(cpu, searchAddresses, searchComparison, start, end, value.toUInt(nullptr, base));
		case SearchType::Int64Type:
			return isSigned ? searchWorker<s64>(cpu, searchAddresses, searchComparison, start, end, value.toLong(nullptr, base)) : searchWorker<s64>(cpu, searchAddresses, searchComparison, start, end, value.toULongLong(nullptr, base));
		case SearchType::FloatType:
			return searchWorker<float>(cpu, searchAddresses, searchComparison, start, end, value.toFloat());
		case SearchType::DoubleType:
			return searchWorker<double>(cpu, searchAddresses, searchComparison, start, end, value.toDouble());
		case SearchType::StringType:
			return searchWorkerByteArray(cpu, searchComparison, searchAddresses, start, end, value.toUtf8());
		case SearchType::ArrayType:
			return searchWorkerByteArray(cpu, searchComparison, searchAddresses, start, end, QByteArray::fromHex(value.toUtf8()));
		default:
			Console.Error("Debugger: Unknown type when doing memory search!");
			break;
	};
	return {};
}

void CpuWidget::onSearchButtonClicked()
{
	if (!m_cpu.isAlive())
		return;

	const SearchType searchType = static_cast<SearchType>(m_ui.cmbSearchType->currentIndex());
	const bool searchHex = m_ui.chkSearchHex->isChecked();

	bool ok;
	const u32 searchStart = m_ui.txtSearchStart->text().toUInt(&ok, 16);

	if (!ok)
	{
		QMessageBox::critical(this, tr("Debugger"), tr("Invalid start address"));
		return;
	}

	const u32 searchEnd = m_ui.txtSearchEnd->text().toUInt(&ok, 16);

	if (!ok)
	{
		QMessageBox::critical(this, tr("Debugger"), tr("Invalid end address"));
		return;
	}

	if (searchStart >= searchEnd)
	{
		QMessageBox::critical(this, tr("Debugger"), tr("Start address can't be equal to or greater than the end address"));
		return;
	}

	const QString searchValue = m_ui.txtSearchValue->text();
	const SearchComparison searchComparison = static_cast<SearchComparison>(m_ui.cmbSearchComparison->currentIndex());
	const bool isFilterSearch = sender() == m_ui.btnFilterSearch;
	unsigned long long value;

	const bool isVariableSize = searchType == SearchType::ArrayType || searchType == SearchType::StringType;
	if (isVariableSize && !isFilterSearch && searchComparison == SearchComparison::NotEquals)
	{
		QMessageBox::critical(this, tr("Debugger"), tr("Search types Array and String can use the Not Equals search comparison type with new searches."));
		return;
	}

	if (isVariableSize && searchComparison != SearchComparison::Equals && searchComparison != SearchComparison::NotEquals)
	{
		QMessageBox::critical(this, tr("Debugger"), tr("Search types Array and String can only be used with Equals search comparisons."));
		return;
	}

	switch (searchType)
	{
		case SearchType::ByteType:
		case SearchType::Int16Type:
		case SearchType::Int32Type:
		case SearchType::Int64Type:
			value = searchValue.toULongLong(&ok, searchHex ? 16 : 10);
			break;
		case SearchType::FloatType:
		case SearchType::DoubleType:
			searchValue.toDouble(&ok);
			break;
		case SearchType::StringType:
			ok = !searchValue.isEmpty();
			break;
		case SearchType::ArrayType:
			ok = !searchValue.trimmed().isEmpty();
			break;
	}

	if (!ok)
	{
		QMessageBox::critical(this, tr("Debugger"), tr("Invalid search value"));
		return;
	}

	switch (searchType)
	{
		case SearchType::ArrayType:
		case SearchType::StringType:
		case SearchType::DoubleType:
		case SearchType::FloatType:
			break;
		case SearchType::Int64Type:
			if (value <= std::numeric_limits<unsigned long long>::max())
				break;
		case SearchType::Int32Type:
			if (value <= std::numeric_limits<unsigned long>::max())
				break;
		case SearchType::Int16Type:
			if (value <= std::numeric_limits<unsigned short>::max())
				break;
		case SearchType::ByteType:
			if (value <= std::numeric_limits<unsigned char>::max())
				break;
		default:
			QMessageBox::critical(this, tr("Debugger"), tr("Value is larger than type"));
			return;
	}

	QFutureWatcher<std::vector<u32>>* workerWatcher = new QFutureWatcher<std::vector<u32>>;

	connect(workerWatcher, &QFutureWatcher<std::vector<u32>>::finished, [this, workerWatcher] {
		m_ui.btnSearch->setDisabled(false);

		m_ui.listSearchResults->clear();
		const auto& results = workerWatcher->future().result();

		m_searchResults = results;
		loadSearchResults();
		m_ui.btnFilterSearch->setDisabled(m_ui.listSearchResults->count() == 0);
	});

	m_ui.btnSearch->setDisabled(true);
	std::vector<u32> addresses;
	if (isFilterSearch)
	{
		addresses = m_searchResults;
	}
	QFuture<std::vector<u32>> workerFuture =
		QtConcurrent::run(startWorker, &m_cpu, searchType, searchComparison, addresses, searchStart, searchEnd, searchValue, searchHex ? 16 : 10);
	workerWatcher->setFuture(workerFuture);
}

void CpuWidget::onSearchResultsListScroll(u32 value)
{
	bool hasResultsToLoad = static_cast<size_t>(m_ui.listSearchResults->count()) < m_searchResults.size();
	bool scrolledSufficiently = value > (m_ui.listSearchResults->verticalScrollBar()->maximum() * 0.95);

	if (!m_resultsLoadTimer.isActive() && hasResultsToLoad && scrolledSufficiently)
	{
		// Load results once timer ends, allowing us to debounce repeated requests and only do one load.
		m_resultsLoadTimer.start();
	}
}

void CpuWidget::loadSearchResults()
{
	const u32 numLoaded = m_ui.listSearchResults->count();
	const u32 amountLeftToLoad = m_searchResults.size() - numLoaded;
	if (amountLeftToLoad < 1)
		return;

	const bool isFirstLoad = numLoaded == 0;
	const u32 maxLoadAmount = isFirstLoad ? m_initialResultsLoadLimit : m_numResultsAddedPerLoad;
	const u32 numToLoad = amountLeftToLoad > maxLoadAmount ? maxLoadAmount : amountLeftToLoad;

	for (u32 i = 0; i < numToLoad; i++)
	{
		u32 address = m_searchResults.at(numLoaded + i);
		QListWidgetItem* item = new QListWidgetItem(QtUtils::FilledQStringFromValue(address, 16));
		item->setData(Qt::UserRole, address);
		m_ui.listSearchResults->addItem(item);
	}
}

void CpuWidget::saveBreakpointsToDebuggerSettings()
{
	DebuggerSettingsManager::saveGameSettings(&m_bpModel);
}

void CpuWidget::saveSavedAddressesToDebuggerSettings()
{
	DebuggerSettingsManager::saveGameSettings(&m_savedAddressesModel);
}
