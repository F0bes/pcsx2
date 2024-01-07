/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2022  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QtWidgets/QMainWindow>

#include "Elfheader.h"
#include "DebugTools/ccc/symbol_database.h"
#include "Models/DataInspectorModel.h"
#include "ui_DataInspectorWindow.h"

class DataInspectorWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit DataInspectorWindow(QWidget* parent = nullptr);

	void resetGlobals();
	void resetStack();
protected:
	static void reportErrorOnUiThread(const ccc::Error& error);
	
	void createGUI();
	std::unique_ptr<DataInspectorNode> populateGlobalSections(
		bool groupBySection, bool groupByTranslationUnit, const QString& filter);
	std::vector<std::unique_ptr<DataInspectorNode>> populateGlobalTranslationUnits(
		u32 minAddress, u32 maxAddress, bool group_by_source_file, const QString& filter);
	std::vector<std::unique_ptr<DataInspectorNode>> populateGlobalVariables(
		const ccc::SourceFile& source_file, u32 minAddress, u32 maxAddress, const QString& filter);
	
	std::unique_ptr<DataInspectorNode> populateStack();
	
	ccc::SymbolDatabase m_database;
	DataInspectorModel* m_globalModel;
	DataInspectorModel* m_stackModel;
	Ui::DataInspectorWindow m_ui;
};