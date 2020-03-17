//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/common/vector_operations/ternary_executor.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/exception.hpp"
#include "duckdb/common/types/vector.hpp"
#include "duckdb/common/vector_operations/vector_operations.hpp"
#include <functional>

namespace duckdb {

struct TernaryExecutor {
private:
	template <class A_TYPE, class B_TYPE, class C_TYPE, class RESULT_TYPE, class FUN>
	static inline void ExecuteLoop(A_TYPE *__restrict adata, B_TYPE *__restrict bdata, C_TYPE *__restrict cdata,
	                               RESULT_TYPE *__restrict result_data, idx_t count,
								   SelectionVector &asel, SelectionVector &bsel, SelectionVector &csel,
								   nullmask_t &anullmask, nullmask_t &bnullmask, nullmask_t &cnullmask,
	                               nullmask_t &result_nullmask, FUN fun) {
		if (anullmask.any() || bnullmask.any() || cnullmask.any()) {
			for(idx_t i = 0; i < count; i++) {
				auto aidx = asel.get_index(i);
				auto bidx = bsel.get_index(i);
				auto cidx = csel.get_index(i);
				if (anullmask[aidx] || bnullmask[bidx] || cnullmask[cidx]) {
					result_nullmask[i] = true;
				} else {
					result_data[i] = fun(adata[aidx], bdata[bidx], cdata[cidx]);
				}
			}
		} else {
			for(idx_t i = 0; i < count; i++) {
				auto aidx = asel.get_index(i);
				auto bidx = bsel.get_index(i);
				auto cidx = csel.get_index(i);
				result_data[i] = fun(adata[aidx], bdata[bidx], cdata[cidx]);
			}
		}
	}
public:
	template <class A_TYPE, class B_TYPE, class C_TYPE, class RESULT_TYPE,
	          class FUN = std::function<RESULT_TYPE(A_TYPE, B_TYPE, C_TYPE)>>
	static void Execute(Vector &a, Vector &b, Vector &c, Vector &result, FUN fun) {
		assert(a.SameCardinality(b) && a.SameCardinality(c) && a.SameCardinality(result));
		if (a.vector_type == VectorType::CONSTANT_VECTOR && b.vector_type == VectorType::CONSTANT_VECTOR && c.vector_type == VectorType::CONSTANT_VECTOR) {
			result.vector_type = VectorType::CONSTANT_VECTOR;
			if (ConstantVector::IsNull(a) || ConstantVector::IsNull(b) || ConstantVector::IsNull(c)) {
				ConstantVector::SetNull(result, true);
			} else {
				auto adata = ConstantVector::GetData<A_TYPE>(a);
				auto bdata = ConstantVector::GetData<B_TYPE>(b);
				auto cdata = ConstantVector::GetData<C_TYPE>(c);
				auto result_data = ConstantVector::GetData<RESULT_TYPE>(result);
				result_data[0] = fun(*adata, *bdata, *cdata);
			}
		} else {
			result.vector_type = VectorType::FLAT_VECTOR;

			VectorData adata, bdata, cdata;
			a.Orrify(adata);
			b.Orrify(bdata);
			c.Orrify(cdata);

			ExecuteLoop<A_TYPE, B_TYPE, C_TYPE, RESULT_TYPE>(
				(A_TYPE*) adata.data,
				(B_TYPE*) bdata.data,
				(C_TYPE*) cdata.data,
				FlatVector::GetData<RESULT_TYPE>(result),
				result.size(),
				*adata.sel,
				*bdata.sel,
				*cdata.sel,
				*adata.nullmask,
				*bdata.nullmask,
				*cdata.nullmask,
				FlatVector::Nullmask(result),
				fun);
		}
	}

private:
	template <class A_TYPE, class B_TYPE, class C_TYPE, class OP>
	static inline idx_t SelectLoop(A_TYPE *__restrict adata, B_TYPE *__restrict bdata, C_TYPE *__restrict cdata,
	                               idx_t count,
								   SelectionVector &asel, SelectionVector &bsel, SelectionVector &csel,
								   nullmask_t &anullmask, nullmask_t &bnullmask, nullmask_t &cnullmask,
								   SelectionVector &true_sel, SelectionVector &false_sel) {
		idx_t true_count = 0, false_count = 0;
		if (anullmask.any() || bnullmask.any() || cnullmask.any()) {
			for(idx_t i = 0; i < count; i++) {
				auto aidx = asel.get_index(i);
				auto bidx = bsel.get_index(i);
				auto cidx = csel.get_index(i);
				if (!anullmask[aidx] && !bnullmask[bidx] && !cnullmask[cidx] &&
				    OP::Operation(adata[aidx], bdata[bidx], cdata[cidx])) {
					true_sel.set_index(true_count++, i);
				} else {
					false_sel.set_index(false_count++, i);
				}
			}
		} else {
			for(idx_t i = 0; i < count; i++) {
				auto aidx = asel.get_index(i);
				auto bidx = bsel.get_index(i);
				auto cidx = csel.get_index(i);
				if (OP::Operation(adata[aidx], bdata[bidx], cdata[cidx])) {
					true_sel.set_index(true_count++, i);
				} else {
					false_sel.set_index(false_count++, i);
				}
			}
		}
		return true_count;
	}
public:
	template <class A_TYPE, class B_TYPE, class C_TYPE, class OP>
	static idx_t Select(Vector &a, Vector &b, Vector &c, SelectionVector &true_sel, SelectionVector &false_sel) {
		assert(a.SameCardinality(b) && a.SameCardinality(c));
		if (a.vector_type == VectorType::CONSTANT_VECTOR && b.vector_type == VectorType::CONSTANT_VECTOR && c.vector_type == VectorType::CONSTANT_VECTOR) {
			auto adata = ConstantVector::GetData<A_TYPE>(a);
			auto bdata = ConstantVector::GetData<B_TYPE>(b);
			auto cdata = ConstantVector::GetData<C_TYPE>(c);
			if (ConstantVector::IsNull(a) || ConstantVector::IsNull(b) ||
				ConstantVector::IsNull(c) || !OP::Operation(*adata, *bdata, *cdata)) {
				return 0;
			} else {
				return a.size();
			}
		} else {
			VectorData adata, bdata, cdata;
			a.Orrify(adata);
			b.Orrify(bdata);
			c.Orrify(cdata);

			return SelectLoop<A_TYPE, B_TYPE, C_TYPE, OP>(
				(A_TYPE*) adata.data,
				(B_TYPE*) bdata.data,
				(C_TYPE*) cdata.data,
				a.size(),
				*adata.sel,
				*bdata.sel,
				*cdata.sel,
				*adata.nullmask,
				*bdata.nullmask,
				*cdata.nullmask,
				true_sel,
				false_sel);
		}
	}
};

} // namespace duckdb
