// SPDX-FileCopyrightText: 2002-2024 PCSX2 Dev Team
// SPDX-License-Identifier: LGPL-3.0+

#include "SymbolTreeValueDelegate.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLineEdit>
#include "Debugger/SymbolTree/SymbolTreeModel.h"

SymbolTreeValueDelegate::SymbolTreeValueDelegate(
	const SymbolGuardian& guardian,
	QObject* parent)
	: QStyledItemDelegate(parent)
	, m_guardian(guardian)
{
}

QWidget* SymbolTreeValueDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QWidget* result = nullptr;

	SymbolTreeNode* node = static_cast<SymbolTreeNode*>(index.internalPointer());
	if (!node->type.valid())
		return result;

	m_guardian.Read([&](const ccc::SymbolDatabase& database) {
		const ccc::ast::Node* logical_type = node->type.lookup_node(database);
		if (!logical_type)
			return;

		const ccc::ast::Node& type = *resolvePhysicalType(logical_type, database).first;
		switch (type.descriptor)
		{
			case ccc::ast::BUILTIN:
			{
				const ccc::ast::BuiltIn& builtIn = type.as<ccc::ast::BuiltIn>();

				switch (builtIn.bclass)
				{
					case ccc::ast::BuiltInClass::UNSIGNED_8:
					case ccc::ast::BuiltInClass::SIGNED_8:
					case ccc::ast::BuiltInClass::UNQUALIFIED_8:
					case ccc::ast::BuiltInClass::UNSIGNED_16:
					case ccc::ast::BuiltInClass::SIGNED_16:
					case ccc::ast::BuiltInClass::UNSIGNED_32:
					case ccc::ast::BuiltInClass::SIGNED_32:
					case ccc::ast::BuiltInClass::FLOAT_32:
					case ccc::ast::BuiltInClass::UNSIGNED_64:
					case ccc::ast::BuiltInClass::SIGNED_64:
					case ccc::ast::BuiltInClass::FLOAT_64:
						result = new QLineEdit(parent);
						break;
					case ccc::ast::BuiltInClass::BOOL_8:
						result = new QCheckBox(parent);
						break;
					default:
					{
					}
				}
				break;
			}
			case ccc::ast::ENUM:
			{
				const ccc::ast::Enum& enumeration = type.as<ccc::ast::Enum>();
				QComboBox* comboBox = new QComboBox(parent);
				for (auto [value, string] : enumeration.constants)
					comboBox->addItem(QString::fromStdString(string));
				result = comboBox;
				break;
			}
			case ccc::ast::POINTER_OR_REFERENCE:
			case ccc::ast::POINTER_TO_DATA_MEMBER:
			{
				result = new QLineEdit(parent);
				break;
			}
			default:
			{
			}
		}
	});

	return result;
}

void SymbolTreeValueDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	SymbolTreeNode* node = static_cast<SymbolTreeNode*>(index.internalPointer());
	if (!node->type.valid())
		return;

	m_guardian.Read([&](const ccc::SymbolDatabase& database) {
		const ccc::ast::Node* logical_type = node->type.lookup_node(database);
		if (!logical_type)
			return;

		const ccc::ast::Node& type = *resolvePhysicalType(logical_type, database).first;
		switch (type.descriptor)
		{
			case ccc::ast::BUILTIN:
			{
				const ccc::ast::BuiltIn& builtIn = type.as<ccc::ast::BuiltIn>();

				switch (builtIn.bclass)
				{
					case ccc::ast::BuiltInClass::UNSIGNED_8:
					case ccc::ast::BuiltInClass::UNQUALIFIED_8:
					case ccc::ast::BuiltInClass::UNSIGNED_16:
					case ccc::ast::BuiltInClass::UNSIGNED_32:
					case ccc::ast::BuiltInClass::UNSIGNED_64:
					{
						QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
						Q_ASSERT(lineEdit);

						lineEdit->setText(QString::number(index.data(Qt::UserRole).toULongLong()));

						break;
					}
					case ccc::ast::BuiltInClass::SIGNED_8:
					case ccc::ast::BuiltInClass::SIGNED_16:
					case ccc::ast::BuiltInClass::SIGNED_32:
					case ccc::ast::BuiltInClass::SIGNED_64:
					{
						QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
						Q_ASSERT(lineEdit);

						lineEdit->setText(QString::number(index.data(Qt::UserRole).toLongLong()));

						break;
					}
					case ccc::ast::BuiltInClass::BOOL_8:
					{
						QCheckBox* checkBox = qobject_cast<QCheckBox*>(editor);
						Q_ASSERT(checkBox);

						checkBox->setChecked(index.data(Qt::UserRole).toBool());

						break;
					}
					case ccc::ast::BuiltInClass::FLOAT_32:
					{
						QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
						Q_ASSERT(lineEdit);

						lineEdit->setText(QString::number(index.data(Qt::UserRole).toFloat()));

						break;
					}
					case ccc::ast::BuiltInClass::FLOAT_64:
					{
						QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
						Q_ASSERT(lineEdit);

						lineEdit->setText(QString::number(index.data(Qt::UserRole).toDouble()));

						break;
					}
					default:
					{
					}
				}
				break;
			}
			case ccc::ast::ENUM:
			{
				const ccc::ast::Enum& enumeration = type.as<ccc::ast::Enum>();
				QComboBox* comboBox = static_cast<QComboBox*>(editor);
				QVariant data = index.data(Qt::UserRole);
				for (s32 i = 0; i < (s32)enumeration.constants.size(); i++)
				{
					if (enumeration.constants[i].first == data.toInt())
					{
						comboBox->setCurrentIndex(i);
						break;
					}
				}
				break;
			}
			case ccc::ast::POINTER_OR_REFERENCE:
			case ccc::ast::POINTER_TO_DATA_MEMBER:
			{
				QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
				Q_ASSERT(lineEdit);

				lineEdit->setText(QString::number(index.data(Qt::UserRole).toULongLong(), 16));

				break;
			}
			default:
			{
			}
		}
	});
}

void SymbolTreeValueDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	SymbolTreeNode* node = static_cast<SymbolTreeNode*>(index.internalPointer());
	if (!node->type.valid())
		return;

	m_guardian.Read([&](const ccc::SymbolDatabase& database) {
		const ccc::ast::Node* logical_type = node->type.lookup_node(database);
		if (!logical_type)
			return;

		const ccc::ast::Node& type = *resolvePhysicalType(logical_type, database).first;
		switch (type.descriptor)
		{
			case ccc::ast::BUILTIN:
			{
				const ccc::ast::BuiltIn& builtIn = type.as<ccc::ast::BuiltIn>();

				switch (builtIn.bclass)
				{
					case ccc::ast::BuiltInClass::UNSIGNED_8:
					case ccc::ast::BuiltInClass::UNQUALIFIED_8:
					case ccc::ast::BuiltInClass::UNSIGNED_16:
					case ccc::ast::BuiltInClass::UNSIGNED_32:
					case ccc::ast::BuiltInClass::UNSIGNED_64:
					{
						QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
						Q_ASSERT(lineEdit);

						bool ok;
						qulonglong value = lineEdit->text().toULongLong(&ok);
						if (ok)
							model->setData(index, value, Qt::UserRole);

						break;
					}
					case ccc::ast::BuiltInClass::SIGNED_8:
					case ccc::ast::BuiltInClass::SIGNED_16:
					case ccc::ast::BuiltInClass::SIGNED_32:
					case ccc::ast::BuiltInClass::SIGNED_64:
					{
						QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
						Q_ASSERT(lineEdit);

						bool ok;
						qlonglong value = lineEdit->text().toLongLong(&ok);
						if (ok)
							model->setData(index, value, Qt::UserRole);

						break;
					}
					case ccc::ast::BuiltInClass::BOOL_8:
					{
						QCheckBox* checkBox = qobject_cast<QCheckBox*>(editor);
						model->setData(index, checkBox->isChecked(), Qt::UserRole);

						break;
					}
					case ccc::ast::BuiltInClass::FLOAT_32:
					{
						QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
						Q_ASSERT(lineEdit);

						bool ok;
						float value = lineEdit->text().toFloat(&ok);
						if (ok)
							model->setData(index, value, Qt::UserRole);

						break;
					}
					case ccc::ast::BuiltInClass::FLOAT_64:
					{
						QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
						Q_ASSERT(lineEdit);

						bool ok;
						double value = lineEdit->text().toDouble(&ok);
						if (ok)
							model->setData(index, value, Qt::UserRole);

						break;
					}
					default:
					{
					}
				}
				break;
			}
			case ccc::ast::ENUM:
			{
				const ccc::ast::Enum& enumeration = type.as<ccc::ast::Enum>();
				QComboBox* comboBox = static_cast<QComboBox*>(editor);
				s32 comboIndex = comboBox->currentIndex();
				if (comboIndex < (s32)enumeration.constants.size())
				{
					s32 value = enumeration.constants[comboIndex].first;
					model->setData(index, QVariant(value), Qt::UserRole);
				}
				break;
			}
			case ccc::ast::POINTER_OR_REFERENCE:
			case ccc::ast::POINTER_TO_DATA_MEMBER:
			{
				QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
				Q_ASSERT(lineEdit);

				bool ok;
				qulonglong address = lineEdit->text().toUInt(&ok, 16);
				if (ok)
					model->setData(index, address, Qt::UserRole);

				break;
			}
			default:
			{
			}
		}
	});
}
