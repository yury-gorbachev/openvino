// Copyright (C) 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "ngraph_ops/convolution_ie.hpp"

#include <algorithm>
#include <memory>
#include <vector>

#include "ngraph/util.hpp"
#include "ngraph/validation_util.hpp"

using namespace std;
using namespace ngraph;

constexpr NodeTypeInfo op::ConvolutionIE::type_info;

op::ConvolutionIE::ConvolutionIE(const Output<Node>& data_batch,
                                 const Output<Node>& filters,
                                 const Strides& strides,
                                 const Strides& dilations,
                                 const CoordinateDiff& pads_begin,
                                 const CoordinateDiff& pads_end,
                                 const size_t& group,
                                 const PadType& auto_pad)
        : Op({data_batch, filters})
        , m_strides(strides)
        , m_dilations(dilations)
        , m_pads_begin(pads_begin)
        , m_pads_end(pads_end)
        , m_auto_pad(auto_pad)
        , m_group(group) {
    constructor_validate_and_infer_types();
}

op::ConvolutionIE::ConvolutionIE(const Output<Node>& data_batch,
                                 const Output<Node>& filters,
                                 const Output<Node>& bias,
                                 const Strides& strides,
                                 const Strides& dilations,
                                 const CoordinateDiff& pads_begin,
                                 const CoordinateDiff& pads_end,
                                 const size_t& group,
                                 const PadType& auto_pad)
        : Op({data_batch, filters, bias})
        , m_strides(strides)
        , m_dilations(dilations)
        , m_pads_begin(pads_begin)
        , m_pads_end(pads_end)
        , m_auto_pad(auto_pad)
        , m_group(group) {
    constructor_validate_and_infer_types();
}

void op::ConvolutionIE::validate_and_infer_types() {
    const PartialShape& data_batch_pshape = get_input_partial_shape(0);
    element::Type data_batch_et = get_input_element_type(0);
    const PartialShape& filters_pshape = get_input_partial_shape(1);
    element::Type filters_et = get_input_element_type(1);

    PartialShape result_shape{PartialShape::dynamic()};

    // we need to adjust filters_shape to reuse helpers for normal convolution
    if (filters_pshape.is_static() && data_batch_pshape.is_static()) {
        auto filters_shape = filters_pshape.to_shape();
        auto groups = m_group;
        auto data_batch_shape = data_batch_pshape.to_shape();
        data_batch_shape[1] /= groups;

        if (m_auto_pad == PadType::SAME_UPPER || m_auto_pad == PadType::SAME_LOWER) {
            m_pads_begin.clear();
            m_pads_end.clear();
            infer_auto_padding(
                data_batch_shape,
                Shape(filters_shape.begin() + 2, filters_shape.end()), // Remove {O,I}
                m_strides,
                m_dilations,
                m_auto_pad,
                m_pads_end,
                m_pads_begin);
        }

        result_shape =
            infer_convolution_forward(this,
                                      data_batch_shape,
                                      Strides(m_strides.size(), 1), // dummy data dilations
                                      m_pads_begin,
                                      m_pads_end,
                                      filters_shape,
                                      m_strides,
                                      m_dilations);
    }
    element::Type result_et;

    NODE_VALIDATION_CHECK(
        this,
        element::Type::merge(result_et, data_batch_et, filters_et),
        "Element types for data batch and filters do not match (data batch element type: ",
        data_batch_et,
        ", filters element type: ",
        filters_et,
        ").");

    set_output_type(0, result_et, result_shape);
}

shared_ptr<Node> op::ConvolutionIE::clone_with_new_inputs(const ngraph::OutputVector & new_args) const {
    if (new_args.size() == 2) {
        return make_shared<ConvolutionIE>(new_args.at(0),
                                          new_args.at(1),
                                          m_strides,
                                          m_dilations,
                                          m_pads_begin,
                                          m_pads_end,
                                          m_group,
                                          m_auto_pad);
    } else if (new_args.size() == 3) {
        return make_shared<ConvolutionIE>(new_args.at(0),
                                          new_args.at(1),
                                          new_args.at(2),
                                          m_strides,
                                          m_dilations,
                                          m_pads_begin,
                                          m_pads_end,
                                          m_group,
                                          m_auto_pad);
    }

    throw ngraph_error("Unsupported number of arguments for ConvolutionIE operation");
}
