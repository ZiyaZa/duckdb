//===----------------------------------------------------------------------===//
//                         DuckDB
//
// execution/operator/schema/physical_create_index.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "execution/physical_operator.hpp"
#include "execution/index/order_index.hpp"
#include "execution/index/art/art.hpp"

#include "storage/data_table.hpp"

#include <fstream>

namespace duckdb {

//! Physically CREATE INDEX statement
class PhysicalCreateIndex : public PhysicalOperator {
public:
	PhysicalCreateIndex(LogicalOperator &op, TableCatalogEntry &table, vector<column_t> column_ids,
	                    vector<unique_ptr<Expression>> expressions, unique_ptr<CreateIndexInformation> info,
	                    vector<unique_ptr<Expression>> unbinded_expressions)
	    : PhysicalOperator(PhysicalOperatorType::CREATE_INDEX, op.types), table(table), column_ids(column_ids),
	      expressions(std::move(expressions)), info(std::move(info)), unbound_expressions(move(unbinded_expressions)) {
	}

	void _GetChunk(ClientContext &context, DataChunk &chunk, PhysicalOperatorState *state) override;

	//! The table to create the index for
	TableCatalogEntry &table;
	//! The list of column IDs required for the index
	vector<column_t> column_ids;
	//! Set of expressions to index by
	vector<unique_ptr<Expression>> expressions;
	//! Info for index creation
	unique_ptr<CreateIndexInformation> info;
	//! Unbinded expressions to be used in the optimizer
	vector<unique_ptr<Expression>> unbound_expressions;

private:
	void createOrderIndex(ScanStructure *ss,DataChunk *intermediate,vector<TypeId> *result_types,DataChunk *result);
	void createARTIndex(ScanStructure *ss,DataChunk *intermediate,vector<TypeId> *result_types,DataChunk *result);

};
} // namespace duckdb
