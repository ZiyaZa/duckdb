#include "duckdb/parser/parsed_data/create_collation_info.hpp"

namespace duckdb {

static unordered_set<string> const equality_requires_collation = {"da"};

CreateCollationInfo::CreateCollationInfo(string name_p, ScalarFunction function_p, bool combinable_p,
                                         bool not_required_for_equality_p)
    : CreateInfo(CatalogType::COLLATION_ENTRY), function(std::move(function_p)), combinable(combinable_p),
      not_required_for_equality(not_required_for_equality_p) {
	this->name = std::move(name_p);
	if (equality_requires_collation.count(this->name) > 0) {
		this->not_required_for_equality = false;
	}
	internal = true;
}

unique_ptr<CreateInfo> CreateCollationInfo::Copy() const {
	auto result = make_uniq<CreateCollationInfo>(name, function, combinable, not_required_for_equality);
	CopyProperties(*result);
	return std::move(result);
}

} // namespace duckdb
