// SPDX-FileCopyrightText: 2002-2024 PCSX2 Dev Team
// SPDX-License-Identifier: LGPL-3.0+

#pragma once

#include <QtCore/QString>
#include <QtCore/QVariant>

#include "SymbolTreeLocation.h"

class DebugInterface;

// A node in a symbol tree model.
struct SymbolTreeNode
{
public:
	enum Tag
	{
		UNKNOWN_GROUP,
		GROUP,
		OBJECT
	};

	Tag tag = OBJECT;
	QString name;
	ccc::NodeHandle type;
	SymbolTreeLocation location;
	ccc::AddressRange live_range;
	ccc::MultiSymbolHandle symbol;
	std::unique_ptr<ccc::ast::Node> temporary_type;

	QString valueToString(DebugInterface& cpu, const ccc::SymbolDatabase& database) const;
	QString valueToString(const ccc::ast::Node& type, DebugInterface& cpu, const ccc::SymbolDatabase& database, s32 depth) const;
	QVariant valueToVariant(DebugInterface& cpu, const ccc::SymbolDatabase& database) const;
	QVariant valueToVariant(const ccc::ast::Node& type, DebugInterface& cpu, const ccc::SymbolDatabase& database) const;
	bool fromVariant(QVariant value, const ccc::ast::Node& type, DebugInterface& cpu) const;

	const SymbolTreeNode* parent() const;

	const std::vector<std::unique_ptr<SymbolTreeNode>>& children() const;
	bool childrenFetched() const;
	void setChildren(std::vector<std::unique_ptr<SymbolTreeNode>> new_children);
	void insertChildren(std::vector<std::unique_ptr<SymbolTreeNode>> new_children);
	void emplaceChild(std::unique_ptr<SymbolTreeNode> new_child);
	void clearChildren();

	void sortChildrenRecursively(bool sort_by_if_type_is_known);

protected:
	SymbolTreeNode* m_parent = nullptr;
	std::vector<std::unique_ptr<SymbolTreeNode>> m_children;
	bool m_children_fetched = false;
};

std::pair<const ccc::ast::Node*, const ccc::DataType*> resolvePhysicalType(const ccc::ast::Node* type, const ccc::SymbolDatabase& database);