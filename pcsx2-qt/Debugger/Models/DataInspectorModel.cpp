#include "DataInspectorModel.h"

DataInspectorModel::DataInspectorModel(
	std::unique_ptr<DataInspectorNode> initialRoot,
	const ccc::SymbolDatabase& database,
	QObject* parent)
	: QAbstractItemModel(parent)
	, m_root(std::move(initialRoot))
	, m_database(database)
{
}

QModelIndex DataInspectorModel::index(int row, int column, const QModelIndex& parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	DataInspectorNode* parentNode;
	if (parent.isValid())
		parentNode = static_cast<DataInspectorNode*>(parent.internalPointer());
	else
		parentNode = m_root.get();

	DataInspectorNode* childNode = parentNode->children.at(row).get();
	if (!childNode)
		return QModelIndex();

	return createIndex(row, column, childNode);
}

QModelIndex DataInspectorModel::parent(const QModelIndex& index) const
{
	if (!index.isValid())
		return QModelIndex();

	DataInspectorNode* childNode = static_cast<DataInspectorNode*>(index.internalPointer());
	DataInspectorNode* parentNode = childNode->parent;
	if (!parentNode)
		return QModelIndex();

	return indexFromNode(*parentNode);
}

int DataInspectorModel::rowCount(const QModelIndex& parent) const
{
	if (parent.column() > 0)
		return 0;

	DataInspectorNode* node;
	if (parent.isValid())
		node = static_cast<DataInspectorNode*>(parent.internalPointer());
	else
		node = m_root.get();

	return (int)node->children.size();
}

int DataInspectorModel::columnCount(const QModelIndex& parent) const
{
	return COLUMN_COUNT;
}

bool DataInspectorModel::hasChildren(const QModelIndex& parent) const
{
	if (!parent.isValid())
		return true;

	DataInspectorNode* parentNode = static_cast<DataInspectorNode*>(parent.internalPointer());
	if (!parentNode->type)
		return true;

	return nodeHasChildren(*parentNode->type);
}

QVariant DataInspectorModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || role != Qt::DisplayRole)
		return QVariant();

	DataInspectorNode* node = static_cast<DataInspectorNode*>(index.internalPointer());

	u32 pc = r5900Debug.getRegister(EECAT_GPR, 32);

	switch (index.column())
	{
		case NAME:
		{
			return node->name;
		}
		case LOCATION:
		{
			return node->location.name();
		}
		case TYPE:
		{
			if (!node->type)
				return QVariant();
			return typeToString(*node->type);
		}
		case LIVENESS:
		{
			//if (node->type && node->type->descriptor == ccc::ast::VARIABLE)
			//{
			//	const ccc::ast::Variable& variable = node->type->as<ccc::ast::Variable>();
			//	if (variable.storage.type != ccc::ast::VariableStorageType::GLOBAL)
			//	{
			//		bool alive = pc >= variable.block.low && pc < variable.block.high;
			//		return alive ? "Alive" : "Dead";
			//	}
			//}
			return QVariant();
		}
		default:
		{
		}
	}

	Q_ASSERT(index.column() == VALUE);

	if (!node->type)
		return QVariant();

	const ccc::ast::Node& type = resolvePhysicalType(*node->type, m_database);

	QVariant result;
	switch (type.descriptor)
	{
		case ccc::ast::BUILTIN:
		{
			const ccc::ast::BuiltIn& builtIn = type.as<ccc::ast::BuiltIn>();
			switch (builtIn.bclass)
			{
				case ccc::ast::BuiltInClass::UNSIGNED_8:
					return (qulonglong)node->location.read8();
				case ccc::ast::BuiltInClass::SIGNED_8:
					return (qlonglong)node->location.read8();
				case ccc::ast::BuiltInClass::UNQUALIFIED_8:
					return (qulonglong)node->location.read8();
				case ccc::ast::BuiltInClass::BOOL_8:
					return (bool)node->location.read8();
				case ccc::ast::BuiltInClass::UNSIGNED_16:
					return (qulonglong)node->location.read16();
				case ccc::ast::BuiltInClass::SIGNED_16:
					return (qlonglong)node->location.read16();
				case ccc::ast::BuiltInClass::UNSIGNED_32:
					return (qulonglong)node->location.read32();
				case ccc::ast::BuiltInClass::SIGNED_32:
					return (qlonglong)node->location.read32();
				case ccc::ast::BuiltInClass::FLOAT_32:
				{
					u32 value = node->location.read32();
					return *reinterpret_cast<float*>(&value);
				}
				case ccc::ast::BuiltInClass::UNSIGNED_64:
					return (qulonglong)node->location.read64();
				case ccc::ast::BuiltInClass::SIGNED_64:
					return (qlonglong)node->location.read64();
				case ccc::ast::BuiltInClass::FLOAT_64:
				{
					u64 value = node->location.read64();
					return *reinterpret_cast<double*>(&value);
				}
				default:
				{
				}
			}
			break;
		}
		case ccc::ast::ENUM:
			return node->location.read32();
		case ccc::ast::POINTER_OR_REFERENCE:
			return node->location.read32();
		default:
		{
		}
	}

	return QVariant();
}

bool DataInspectorModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (!index.isValid())
		return false;

	DataInspectorNode* node = static_cast<DataInspectorNode*>(index.internalPointer());
	if (!node->type)
		return false;

	const ccc::ast::Node& type = resolvePhysicalType(*node->type, m_database);
	switch (type.descriptor)
	{
		case ccc::ast::BUILTIN:
		{
			const ccc::ast::BuiltIn& builtIn = type.as<ccc::ast::BuiltIn>();

			switch (builtIn.bclass)
			{
				case ccc::ast::BuiltInClass::UNSIGNED_8:
					node->location.write8((u8)value.toULongLong());
					break;
				case ccc::ast::BuiltInClass::SIGNED_8:
					node->location.write8((u8)value.toLongLong());
					break;
				case ccc::ast::BuiltInClass::UNQUALIFIED_8:
					node->location.write8((u8)value.toULongLong());
					break;
				case ccc::ast::BuiltInClass::BOOL_8:
					node->location.write8((u8)value.toBool());
					break;
				case ccc::ast::BuiltInClass::UNSIGNED_16:
					node->location.write16((u16)value.toULongLong());
					break;
				case ccc::ast::BuiltInClass::SIGNED_16:
					node->location.write16((u16)value.toLongLong());
					break;
				case ccc::ast::BuiltInClass::UNSIGNED_32:
					node->location.write32((u32)value.toULongLong());
					break;
				case ccc::ast::BuiltInClass::SIGNED_32:
					node->location.write32((u32)value.toLongLong());
					break;
				case ccc::ast::BuiltInClass::FLOAT_32:
				{
					float f = value.toFloat();
					node->location.write32(*reinterpret_cast<u32*>(&f));
					break;
				}
				case ccc::ast::BuiltInClass::UNSIGNED_64:
					node->location.write64((u64)value.toULongLong());
					break;
				case ccc::ast::BuiltInClass::SIGNED_64:
					node->location.write64((u64)value.toLongLong());
					break;
				case ccc::ast::BuiltInClass::FLOAT_64:
				{
					double d = value.toDouble();
					node->location.write64(*reinterpret_cast<u64*>(&d));
					break;
				}
				default:
				{
					return false;
				}
			}
			break;
		}
		case ccc::ast::ENUM:
			node->location.write32((u32)value.toULongLong());
			break;
		case ccc::ast::POINTER_OR_REFERENCE:
			node->location.write32((u32)value.toULongLong());
			break;
		default:
		{
			return false;
		}
	}

	emit dataChanged(index, index);

	return true;
}

void DataInspectorModel::fetchMore(const QModelIndex& parent)
{
	if (!parent.isValid())
		return;

	DataInspectorNode* parentNode = static_cast<DataInspectorNode*>(parent.internalPointer());
	if (!parentNode->type)
		return;

	const ccc::ast::Node& parentType = resolvePhysicalType(*parentNode->type, m_database);

	std::vector<std::unique_ptr<DataInspectorNode>> children;
	switch (parentType.descriptor)
	{
		case ccc::ast::ARRAY:
		{
			const ccc::ast::Array& array = parentType.as<ccc::ast::Array>();
			for (s32 i = 0; i < array.element_count; i++)
			{
				std::unique_ptr<DataInspectorNode> element = std::make_unique<DataInspectorNode>();
				element->name = QString("[%1]").arg(i);
				element->type = array.element_type.get();
				element->location = parentNode->location.addOffset(i * array.element_type->computed_size_bytes);
				children.emplace_back(std::move(element));
			}
			break;
		}
		case ccc::ast::POINTER_OR_REFERENCE:
		{
			u32 address = parentNode->location.read32();
			if (parentNode->location.cpu().isValidAddress(address))
			{
				const ccc::ast::PointerOrReference& pointerOrReference = parentType.as<ccc::ast::PointerOrReference>();
				std::unique_ptr<DataInspectorNode> element = std::make_unique<DataInspectorNode>();
				element->name = QString("*%1").arg(address);
				element->type = pointerOrReference.value_type.get();
				element->location = parentNode->location.createAddress(address);
				children.emplace_back(std::move(element));
			}
			break;
		}
		case ccc::ast::STRUCT_OR_UNION:
		{
			const ccc::ast::StructOrUnion& structOrUnion = parentType.as<ccc::ast::StructOrUnion>();
			for (const std::unique_ptr<ccc::ast::Node>& field : structOrUnion.fields)
			{
				std::unique_ptr<DataInspectorNode> childNode = std::make_unique<DataInspectorNode>();
				childNode->name = QString::fromStdString(field->name);
				childNode->type = field.get();
				childNode->location = parentNode->location.addOffset(field->offset_bytes);
				children.emplace_back(std::move(childNode));
			}
			break;
		}
		default:
		{
		}
	}

	if (children.empty())
	{
		parentNode->childrenFetched = true;
		return;
	}

	for (std::unique_ptr<DataInspectorNode>& childNode : children)
		childNode->parent = parentNode;

	beginInsertRows(parent, 0, children.size() - 1);
	parentNode->children = std::move(children);
	parentNode->childrenFetched = true;
	endInsertRows();
}

bool DataInspectorModel::canFetchMore(const QModelIndex& parent) const
{
	if (!parent.isValid())
		return false;

	DataInspectorNode* parentNode = static_cast<DataInspectorNode*>(parent.internalPointer());
	if (!parentNode->type)
		return false;

	return nodeHasChildren(*parentNode->type) && !parentNode->childrenFetched;
}

Qt::ItemFlags DataInspectorModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	Qt::ItemFlags flags = QAbstractItemModel::flags(index);

	if (index.column() == VALUE)
		flags |= Qt::ItemIsEditable;

	return flags;
}

QVariant DataInspectorModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return QVariant();

	switch (section)
	{
		case NAME:
		{
			return "Name";
		}
		case LOCATION:
		{
			return "Location";
		}
		case TYPE:
		{
			return "Type";
		}
		case LIVENESS:
		{
			return "Liveness";
		}
		case VALUE:
		{
			return "Value";
		}
	}

	return QVariant();
}

void DataInspectorModel::reset(std::unique_ptr<DataInspectorNode> newRoot)
{
	beginResetModel();
	m_root = std::move(newRoot);
	endResetModel();
}

bool DataInspectorModel::nodeHasChildren(const ccc::ast::Node& type) const
{
	const ccc::ast::Node& physicalType = resolvePhysicalType(type, m_database);

	bool result = false;
	switch (physicalType.descriptor)
	{
		case ccc::ast::ARRAY:
		{
			const ccc::ast::Array& array = physicalType.as<ccc::ast::Array>();
			result = array.element_count > 0;
			break;
		}
		case ccc::ast::POINTER_OR_REFERENCE:
		{
			result = true;
			break;
		}
		case ccc::ast::STRUCT_OR_UNION:
		{
			const ccc::ast::StructOrUnion& structOrUnion = physicalType.as<ccc::ast::StructOrUnion>();
			result = !structOrUnion.fields.empty() || !structOrUnion.base_classes.empty();
			break;
		}
		default:
		{
		}
	}

	return result;
}

QModelIndex DataInspectorModel::indexFromNode(const DataInspectorNode& node) const
{
	int row = 0;
	if (node.parent)
		for (int i = 0; i < (int)node.parent->children.size(); i++)
		{
			if (node.parent->children[i].get() == &node)
				row = i;
		}
	else
		row = 0;

	return createIndex(row, 0, &node);
}

QString DataInspectorModel::typeToString(const ccc::ast::Node& type) const
{
	QString result;

	switch (type.descriptor)
	{
		case ccc::ast::TYPE_NAME:
		{
			const ccc::ast::TypeName& type_name = type.as<ccc::ast::TypeName>();
			const ccc::DataType* data_type = m_database.data_types.symbol_from_handle(type_name.data_type_handle);
			if(data_type) {
				result = QString::fromStdString(data_type->name());
			}
			break;
		}
		default:
		{
			result = ccc::ast::node_type_to_string(type);
		}
	}

	return result;
}

const ccc::ast::Node& resolvePhysicalType(const ccc::ast::Node& type, const ccc::SymbolDatabase& database)
{
	const ccc::ast::Node* result = &type;
	for (s32 i = 0; i < 10 && result->descriptor == ccc::ast::TYPE_NAME; i++)
	{
		const ccc::DataType* symbol = database.data_types.symbol_from_handle(type.as<ccc::ast::TypeName>().data_type_handle);
		if (!symbol)
		{
			break;
		}
		result = symbol->type();
	}
	return *result;
}