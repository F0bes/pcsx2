// SPDX-FileCopyrightText: 2002-2023 PCSX2 Dev Team
// SPDX-License-Identifier: LGPL-3.0+

#pragma once

#include <QtWidgets/QWidget>
#include "SymbolTreeModel.h"

#include "ui_SymbolTreeWidget.h"

struct SymbolFilters;

// A symbol tree widget with its associated refresh button, filter box and
// right-click menu. Supports grouping, sorting and various other settings.
class SymbolTreeWidget : public QWidget
{
	Q_OBJECT

public:
	virtual ~SymbolTreeWidget();

	void update();

signals:
	void goToInDisassembly(u32 address);
	void goToInMemoryView(u32 address);
	void nameColumnClicked(u32 address);
	void locationColumnClicked(u32 address);

protected:
	struct SymbolWork
	{
		QString name;
		ccc::SymbolDescriptor descriptor;
		const ccc::Symbol* symbol = nullptr;
		const ccc::Module* module_symbol = nullptr;
		const ccc::Section* section = nullptr;
		const ccc::SourceFile* source_file = nullptr;
	};

	explicit SymbolTreeWidget(u32 flags, s32 symbol_address_alignment, DebugInterface& cpu, QWidget* parent = nullptr);

	void setupTree();
	std::unique_ptr<SymbolTreeNode> buildTree(const SymbolFilters& filters, const ccc::SymbolDatabase& database);

	std::unique_ptr<SymbolTreeNode> groupBySourceFile(
		std::unique_ptr<SymbolTreeNode> child, const SymbolWork& child_work, SymbolTreeNode*& prev_group, const SymbolWork*& prev_work);
	std::unique_ptr<SymbolTreeNode> groupBySection(
		std::unique_ptr<SymbolTreeNode> child, const SymbolWork& child_work, SymbolTreeNode*& prev_group, const SymbolWork*& prev_work);
	std::unique_ptr<SymbolTreeNode> groupByModule(
		std::unique_ptr<SymbolTreeNode> child, const SymbolWork& child_work, SymbolTreeNode*& prev_group, const SymbolWork*& prev_work);

	void setupMenu();
	void openMenu(QPoint pos);

	virtual std::vector<SymbolWork> getSymbols(
		const QString& filter, const ccc::SymbolDatabase& database) = 0;

	virtual std::unique_ptr<SymbolTreeNode> buildNode(
		SymbolWork& work, const ccc::SymbolDatabase& database) const = 0;

	virtual void configureColumns() = 0;

	virtual void onNewButtonPressed() = 0;
	virtual void onDeleteButtonPressed() = 0;

	void onCopyName();
	void onCopyLocation();
	void onRenameSymbol();
	void onGoToInDisassembly();
	void onGoToInMemoryView();
	void onResetChildren();
	void onChangeTypeTemporarily();

	bool currentNodeIsObject();
	bool currentNodeIsSymbol();

	void onTreeViewClicked(const QModelIndex& index);

	SymbolTreeNode* currentNode();

	Ui::SymbolTreeWidget m_ui;

	DebugInterface& m_cpu;
	SymbolTreeModel* m_model = nullptr;

	QMenu* m_context_menu = nullptr;
	QAction* m_rename_symbol = nullptr;
	QAction* m_group_by_module = nullptr;
	QAction* m_group_by_section = nullptr;
	QAction* m_group_by_source_file = nullptr;
	QAction* m_sort_by_if_type_is_known = nullptr;
	QAction* m_reset_children = nullptr;
	QAction* m_change_type_temporarily = nullptr;

	enum Flags
	{
		NO_SYMBOL_TREE_FLAGS = 0,
		ALLOW_GROUPING = 1 << 0,
		ALLOW_SORTING_BY_IF_TYPE_IS_KNOWN = 1 << 1,
		ALLOW_TYPE_ACTIONS = 1 << 2
	};

	u32 m_flags;
	u32 m_symbol_address_alignment;
};

class FunctionTreeWidget : public SymbolTreeWidget
{
	Q_OBJECT
public:
	explicit FunctionTreeWidget(DebugInterface& cpu, QWidget* parent = nullptr);
	virtual ~FunctionTreeWidget();

protected:
	std::vector<SymbolWork> getSymbols(
		const QString& filter, const ccc::SymbolDatabase& database) override;

	std::unique_ptr<SymbolTreeNode> buildNode(
		SymbolWork& work, const ccc::SymbolDatabase& database) const override;

	void configureColumns() override;

	void onNewButtonPressed() override;
	void onDeleteButtonPressed() override;
};

class GlobalVariableTreeWidget : public SymbolTreeWidget
{
	Q_OBJECT
public:
	explicit GlobalVariableTreeWidget(DebugInterface& cpu, QWidget* parent = nullptr);
	virtual ~GlobalVariableTreeWidget();

protected:
	std::vector<SymbolWork> getSymbols(
		const QString& filter, const ccc::SymbolDatabase& database) override;

	std::unique_ptr<SymbolTreeNode> buildNode(
		SymbolWork& work, const ccc::SymbolDatabase& database) const override;

	void configureColumns() override;

	void onNewButtonPressed() override;
	void onDeleteButtonPressed() override;
};

class LocalVariableTreeWidget : public SymbolTreeWidget
{
	Q_OBJECT
public:
	explicit LocalVariableTreeWidget(DebugInterface& cpu, QWidget* parent = nullptr);
	virtual ~LocalVariableTreeWidget();

protected:
	std::vector<SymbolWork> getSymbols(
		const QString& filter, const ccc::SymbolDatabase& database) override;

	std::unique_ptr<SymbolTreeNode> buildNode(
		SymbolWork& work, const ccc::SymbolDatabase& database) const override;

	void configureColumns() override;

	void onNewButtonPressed() override;
	void onDeleteButtonPressed() override;

	u32 m_stack_pointer = 0;
};

class ParameterVariableTreeWidget : public SymbolTreeWidget
{
	Q_OBJECT
public:
	explicit ParameterVariableTreeWidget(DebugInterface& cpu, QWidget* parent = nullptr);
	virtual ~ParameterVariableTreeWidget();

protected:
	std::vector<SymbolWork> getSymbols(
		const QString& filter, const ccc::SymbolDatabase& database) override;

	std::unique_ptr<SymbolTreeNode> buildNode(
		SymbolWork& work, const ccc::SymbolDatabase& database) const override;

	void configureColumns() override;

	void onNewButtonPressed() override;
	void onDeleteButtonPressed() override;

	u32 m_stack_pointer = 0;
};

struct SymbolFilters
{
	bool group_by_module = false;
	bool group_by_section = false;
	bool group_by_source_file = false;
	QString string;
};