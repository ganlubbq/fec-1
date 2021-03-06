/*
Copyright (C) 2017 Thiemar Pty Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "FEC/Types.h"
#include "FEC/Utilities.h"
#include "FEC/BinarySequence.h"

namespace Thiemar {

namespace Detail {
    /* Helper functions for interleaver. */
    template <std::size_t NumPoly, std::size_t PolyIndex, std::size_t Count, typename PuncturingMatrix, bool... RowBits>
    struct PuncturingMatrixRow;

    template <std::size_t NumPoly, std::size_t PolyIndex, std::size_t Count, bool... Bits, bool... RowBits>
    struct PuncturingMatrixRow<NumPoly, PolyIndex, Count, BinarySequence<Bits...>, RowBits...> {
        using type = typename PuncturingMatrixRow<NumPoly, PolyIndex, Count-1u, BinarySequence<Bits...>,
            RowBits..., BinarySequence<Bits...>::template test<
            (BinarySequence<Bits...>::size() / NumPoly - 1u - Count) * NumPoly + PolyIndex>()>::type;
    };

    template <std::size_t NumPoly, std::size_t PolyIndex, bool... Bits, bool... RowBits>
    struct PuncturingMatrixRow<NumPoly, PolyIndex, 0u, BinarySequence<Bits...>, RowBits...> {
        using type = BinarySequence<RowBits...,
            BinarySequence<Bits...>::template test<
            (BinarySequence<Bits...>::size() / NumPoly - 1u) * NumPoly + PolyIndex>()>;
    };

    template <std::size_t NumPoly, std::size_t PolyIndex, std::size_t N, bool... Bits>
    constexpr std::size_t wrapped_input_index(BinarySequence<Bits...>) {
        /* Get this row of the puncturing matrix. */
        using puncturing_matrix_row = typename PuncturingMatrixRow<
            NumPoly, PolyIndex, BinarySequence<Bits...>::size() / NumPoly - 1u, BinarySequence<Bits...>>::type;
        
        return (N / puncturing_matrix_row::ones()) * puncturing_matrix_row::size() +
            get_index<N % puncturing_matrix_row::ones()>((typename puncturing_matrix_row::ones_index_sequence){});
    }

    template <std::size_t NumPoly, std::size_t PolyIndex, std::size_t N, bool... Bits>
    constexpr std::size_t wrapped_output_index(BinarySequence<Bits...>) {
        /* Get this row of the puncturing matrix. */
        using puncturing_matrix_row = typename PuncturingMatrixRow<
            NumPoly, PolyIndex, BinarySequence<Bits...>::size() / NumPoly - 1u, BinarySequence<Bits...>>::type;

        return (N / puncturing_matrix_row::ones()) * BinarySequence<Bits...>::ones() +
            BinarySequence<Bits...>::template sum<get_index<N % puncturing_matrix_row::ones()>(
            (typename puncturing_matrix_row::ones_index_sequence){}) * NumPoly + PolyIndex>() - PolyIndex;
    }

    template <typename PuncturingMatrix, std::size_t NumPoly, std::size_t PolyIndex, typename Seq>
    struct UnwrappedInputIndexSequence;

    template <typename PuncturingMatrix, std::size_t NumPoly, std::size_t PolyIndex, std::size_t... N>
    struct UnwrappedInputIndexSequence<PuncturingMatrix, NumPoly, PolyIndex, std::index_sequence<N...>> {
        using type = std::index_sequence<wrapped_input_index<NumPoly, PolyIndex, N>(PuncturingMatrix{})...>;
    };

    template <typename PuncturingMatrix, std::size_t NumPoly, std::size_t PolyIndex, typename Seq>
    struct UnwrappedOutputIndexSequence;

    template <typename PuncturingMatrix, std::size_t NumPoly, std::size_t PolyIndex, std::size_t... N>
    struct UnwrappedOutputIndexSequence<PuncturingMatrix, NumPoly, PolyIndex, std::index_sequence<N...>> {
        using type = std::index_sequence<wrapped_output_index<NumPoly, PolyIndex, N>(PuncturingMatrix{})...>;
    };

    /*
    Use magic numbers to spread input word as efficiently as possible based
    on the puncturing matrix.
    */

    /* This is the final shift/mask operation. */
    template <std::size_t ShiftIndex, std::size_t... InputIndices, std::size_t... DiffIndices>
    bool_vec_t spread_word(bool_vec_t in, std::index_sequence<InputIndices...>, std::index_sequence<DiffIndices...>) {
        /*
        Add ShiftIndex to the InputIndices which correspond to DiffIndices
        greater than or equal to ShiftIndex.
        */
        using new_input_sequence = std::index_sequence<DiffIndices & ShiftIndex ? InputIndices + ShiftIndex : InputIndices...>;

        /*
        Subtract ShiftIndex from DiffIndices which are greater than or equal
        to ShiftIndex.
        */
        using new_diff_sequence = std::index_sequence<DiffIndices & ShiftIndex ? DiffIndices - ShiftIndex : DiffIndices...>;

        /* Generate masks. */
        constexpr bool_vec_t mask_shift = mask_from_index_sequence(
            std::index_sequence<DiffIndices & ShiftIndex ? InputIndices : sizeof(bool_vec_t)*8u ...>{});
        constexpr bool_vec_t mask_static = mask_from_index_sequence(
            std::index_sequence<DiffIndices < ShiftIndex ? InputIndices : sizeof(bool_vec_t)*8u ...>{});

        if constexpr (ShiftIndex == 1u) {
            return (in & mask_static) | (in & mask_shift) >> ShiftIndex;
        } else if constexpr (index_sequence_below_threshold(ShiftIndex, std::index_sequence<DiffIndices...>{})) {
            /*
            Skip if none of the DiffIndices are greater than or equal to
            ShiftIndex.
            */
            return spread_word<ShiftIndex / 2u>(in, std::index_sequence<InputIndices...>{}, std::index_sequence<DiffIndices...>{});
        } else {
            /*
            Create the mask from InputIndices and apply it after the shift
            operation.
            */
            return spread_word<ShiftIndex / 2u>((in & mask_static) | (in & mask_shift) >> ShiftIndex,
                new_input_sequence{}, new_diff_sequence{});
        }
    }

    /*
    Use magic numbers to despread input word as efficiently as possible based
    on the puncturing matrix.
    */

    /* This is the final shift/mask operation. */
    template <std::size_t ShiftIndex, std::size_t... OutputIndices, std::size_t... DiffIndices>
    bool_vec_t despread_word(bool_vec_t in, std::index_sequence<OutputIndices...>, std::index_sequence<DiffIndices...>) {
        /*
        Add ShiftIndex to the OutputIndices which correspond to DiffIndices
        greater than or equal to ShiftIndex.
        */
        using new_output_sequence = std::index_sequence<DiffIndices & ShiftIndex ? OutputIndices - ShiftIndex : OutputIndices...>;

        /*
        Subtract ShiftIndex from DiffIndices which are greater than or equal
        to ShiftIndex.
        */
        using new_diff_sequence = std::index_sequence<DiffIndices & ShiftIndex ? DiffIndices - ShiftIndex : DiffIndices...>;

        /* Generate masks. */
        constexpr bool_vec_t mask_shift = mask_from_index_sequence(
            std::index_sequence<DiffIndices & ShiftIndex ? OutputIndices : sizeof(bool_vec_t)*8u ...>{});
        constexpr bool_vec_t mask_static = mask_from_index_sequence(
            std::index_sequence<!(DiffIndices & ShiftIndex) ? OutputIndices : sizeof(bool_vec_t)*8u ...>{});

        if constexpr (ShiftIndex == sizeof(bool_vec_t) * 8u / 2u) {
            return (in & mask_static) | (in & mask_shift) << ShiftIndex;
        } else if constexpr (index_sequence_below_threshold(ShiftIndex, std::index_sequence<DiffIndices...>{})) {
            /*
            Skip if none of the DiffIndices are greater than or equal to
            ShiftIndex.
            */
            return despread_word<ShiftIndex * 2u>(in, std::index_sequence<OutputIndices...>{}, std::index_sequence<DiffIndices...>{});
        } else {
            /*
            Create the mask from OutputIndices and apply it after the shift
            operation.
            */
            return despread_word<ShiftIndex * 2u>((in & mask_static) | (in & mask_shift) << ShiftIndex,
                new_output_sequence{}, new_diff_sequence{});
        }
    }
}

namespace Convolutional {

/* Class to do arbitrary bitwise interleaving and deinterleaving. */
template <typename PuncturingMatrix, std::size_t NumPoly, std::size_t BlockSize>
class Interleaver {
    static_assert(BlockSize > 0u, "Block size must be at least 1 byte");
    static_assert(NumPoly > 1u, "Minimum of two polynomials are required");
    static_assert(PuncturingMatrix::size() % NumPoly == 0u,
        "Puncturing matrix size must be an integer multiple of the code rate");
    static_assert(PuncturingMatrix::size() > 0u, "Puncturing matrix size must be larger than zero");
    static_assert(sizeof(bool_vec_t) * 8u / PuncturingMatrix::ones() > 0u,
        "Word size must be large enough to fit at least one puncturing matrix cycle");
    static_assert(!(BlockSize % (PuncturingMatrix::size() / NumPoly)),
        "Block size must correspond to an integer number of puncturing matrix cycles");

    /* Calculate number of bits per stream consumed per interleaver cycle. */
    static constexpr std::size_t num_in_bits() {
        constexpr std::size_t max_bits = sizeof(bool_vec_t) * 8u * (PuncturingMatrix::size() / NumPoly) /
            PuncturingMatrix::ones();

        return (max_bits / (PuncturingMatrix::size() / NumPoly)) * (PuncturingMatrix::size() / NumPoly);
    }

    /*
    Calculate number of bits generated by a given polynomial per interleaver
    cycle.
    */
    template <std::size_t PolyIndex>
    static constexpr std::size_t num_poly_bits() {
        using puncturing_matrix_row = typename Detail::PuncturingMatrixRow<
            NumPoly, PolyIndex, PuncturingMatrix::size() / NumPoly - 1u, PuncturingMatrix>::type;

        return (num_in_bits() / (PuncturingMatrix::size() / NumPoly)) * puncturing_matrix_row::ones();
    }

    /* Calculate number of output bits produced per interleaver cycle. */
    static constexpr std::size_t num_out_bits() {
        return (num_in_bits() * PuncturingMatrix::ones()) / (PuncturingMatrix::size() / NumPoly);
    }

    /* Number of interleaver iterations required to complete a block. */
    static constexpr std::size_t num_iterations() {
        return std::max(BlockSize * 8u / num_in_bits() + ((BlockSize * 8u % num_in_bits()) ? 1u : 0u), (std::size_t)1u);
    }

    template <std::size_t PolyIndex>
    using input_index_sequence = typename Detail::UnwrappedInputIndexSequence<PuncturingMatrix, NumPoly, PolyIndex,
        std::make_index_sequence<Interleaver::num_poly_bits<PolyIndex>()>>::type;

    template <std::size_t PolyIndex>
    using output_index_sequence = typename Detail::UnwrappedOutputIndexSequence<PuncturingMatrix, NumPoly, PolyIndex,
        std::make_index_sequence<Interleaver::num_poly_bits<PolyIndex>()>>::type;

    /* Interleaves a block of bool_vec_t. */
    template <std::size_t N, std::size_t... I, std::size_t... O>
    static std::array<uint8_t, sizeof...(O)> interleave_block(const std::array<bool_vec_t, N> &in,
            std::index_sequence<I...>, std::index_sequence<O...>) {
        /* Pack bits into input vector and interleave. */
        std::array<bool_vec_t, sizeof...(I)> out_vec = { pack_and_spread_vec<I>(in, std::make_index_sequence<NumPoly>{}) ... };

        /* Pack interleaved bits into output buffer. */
        return std::array<uint8_t, sizeof...(O)>{ pack_out_vec<O>(out_vec) ... };
    }

    /*
    Pack bits from 'in' into a bool_vec_t array such that they are aligned
    correctly, and then call spread_words.
    */
    template <std::size_t I, std::size_t N, std::size_t... PolyIndices>
    static bool_vec_t pack_and_spread_vec(const std::array<bool_vec_t, N> &in, std::index_sequence<PolyIndices...>) {
        constexpr std::size_t coarse_offset = ((I * num_in_bits()) / (sizeof(bool_vec_t) * 8u)) * NumPoly;
        constexpr std::size_t fine_offset = (I * num_in_bits()) % (sizeof(bool_vec_t) * 8u);

        std::array<bool_vec_t, NumPoly> in_vec = { (bool_vec_t)(in[coarse_offset + PolyIndices] << fine_offset) ... };

        if constexpr (coarse_offset < in_buf_len() - NumPoly && fine_offset) {
            ((in_vec[PolyIndices] |= in[coarse_offset + PolyIndices + NumPoly] >> (sizeof(bool_vec_t) * 8u - fine_offset)), ...);
        }

        constexpr bool_vec_t mask = Detail::mask_bits(num_in_bits());
        ((in_vec[PolyIndices] &= mask), ...);

        return spread_words(in_vec, std::index_sequence<PolyIndices...>{});
    }

    /* Pack 8 bits from 'in' into a single output byte. */
    template <std::size_t O, std::size_t N>
    static uint8_t pack_out_vec(const std::array<bool_vec_t, N> &in) {
        constexpr std::size_t coarse_offset = (O * 8u) / num_out_bits();
        constexpr std::size_t fine_offset = (O * 8u) % num_out_bits();

        constexpr bool_vec_t mask = Detail::mask_bits(num_out_bits());

        uint8_t out;
        if constexpr (fine_offset <= (sizeof(bool_vec_t)-1u) * 8u) {
            out = (in[coarse_offset] & mask) >> ((sizeof(bool_vec_t)-1u) * 8u - fine_offset);
        } else {
            out = (in[coarse_offset] & mask) << (fine_offset - (sizeof(bool_vec_t)-1u) * 8u);
        }

        if constexpr (num_out_bits() - fine_offset < 8u) {
            out |= in[coarse_offset+1u] >> ((sizeof(bool_vec_t)-1u) * 8u + num_out_bits() - fine_offset);
        }

        return out;
    }

    /*
    Packs an array of bool_vec_t of size NumPoly into a single bool_vec_t.

    For this to work properly, only the N most significant bits of each
    element of in should be populated, where N is the number of bits in
    bool_vec_t multiplied by the code rate.
    */
    template <std::size_t... PolyIndices>
    static bool_vec_t spread_words(const std::array<bool_vec_t, NumPoly> &in, std::index_sequence<PolyIndices...>) {
        bool_vec_t out = (... | (Detail::spread_word<sizeof(bool_vec_t) * 8u / 2u>(
            in[PolyIndices], input_index_sequence<PolyIndices>{},
            (typename Detail::DiffIndexSequence<output_index_sequence<PolyIndices>,
            input_index_sequence<PolyIndices>>::type){}) >> PolyIndices));

        return out;
    }

    /* Deinterleaves a block of bytes. */
    template <std::size_t N, std::size_t... I, std::size_t... O>
    static std::array<bool_vec_t, N> deinterleave_block(const std::array<uint8_t, sizeof...(I)> &in,
            std::index_sequence<I...>, std::index_sequence<O...>) {
        std::array<bool_vec_t, sizeof...(O)> in_vec = {};

        /* Pack data from input buffer into bool_vec_t. */
        (unpack_in_vec<I>(in[I], in_vec), ...);

        /* Deinterleave and pack bits into output vector. */
        std::array<bool_vec_t, N> out = {};
        (despread_and_pack_vector<O>(in_vec, out, std::make_index_sequence<NumPoly>{}), ...);

        return out;
    }

    /* Unpack a byte from 'in' into a bool_vec_t at the appropriate index. */
    template <std::size_t I, std::size_t N>
    static void unpack_in_vec(uint8_t in, std::array<bool_vec_t, N> &in_vec) {
        constexpr std::size_t coarse_offset = (I * 8u) / num_out_bits();
        constexpr std::size_t fine_offset = (I * 8u) % num_out_bits();

        constexpr bool_vec_t mask = Detail::mask_bits(num_out_bits());

        if constexpr (fine_offset <= (sizeof(bool_vec_t)-1u) * 8u) {
            in_vec[coarse_offset] |= ((bool_vec_t)in << ((sizeof(bool_vec_t)-1u) * 8u - fine_offset)) & mask;
        } else {
            in_vec[coarse_offset] |= ((bool_vec_t)in >> (fine_offset - (sizeof(bool_vec_t)-1u) * 8u)) & mask;
        }

        if constexpr (num_out_bits() - fine_offset < 8u) {
            in_vec[coarse_offset+1u] |= (bool_vec_t)in << ((sizeof(bool_vec_t)-1u) * 8u + num_out_bits() - fine_offset);
        }
    }

    /*
    Deinterleave bits from the input vector, and then pack them into the
    array of output vectors.
    */
    template <std::size_t O, std::size_t N, std::size_t M, std::size_t... PolyIndices>
    static void despread_and_pack_vector(const std::array<bool_vec_t, N> &in_vec, std::array<bool_vec_t, M> &out,
            std::index_sequence<PolyIndices...>) {
        std::array<bool_vec_t, NumPoly> in_despread = { Detail::despread_word<1u>(
            in_vec[O] << PolyIndices, output_index_sequence<PolyIndices>{},
            (typename Detail::DiffIndexSequence<output_index_sequence<PolyIndices>,
            input_index_sequence<PolyIndices>>::type){}) ... };

        constexpr std::size_t coarse_offset = ((O * num_in_bits()) / (sizeof(bool_vec_t) * 8u)) * NumPoly;
        constexpr std::size_t fine_offset = (O * num_in_bits()) % (sizeof(bool_vec_t) * 8u);

        ((out[coarse_offset + PolyIndices] |= in_despread[PolyIndices] >> fine_offset), ...);

        if constexpr (coarse_offset < in_buf_len() - NumPoly && fine_offset) {
            ((out[coarse_offset + PolyIndices + NumPoly] |= in_despread[PolyIndices] << (sizeof(bool_vec_t) * 8u - fine_offset)), ...);
        }
    }

public:
    /*
    Size (in bool_vec_t) of the input buffer required to supply a full
    interleave block operation.
    */
    static constexpr std::size_t in_buf_len() {
        return NumPoly * (BlockSize / sizeof(bool_vec_t) + ((BlockSize % sizeof(bool_vec_t)) ? 1u : 0u));
    }

    /*
    Size (in bytes) of the output buffer filled by a full interleave block
    operation.
    */
    static constexpr std::size_t out_buf_len() {
        return BlockSize * PuncturingMatrix::ones() / (PuncturingMatrix::size() / NumPoly);
    }

    /* Interleaves a block of bool_vec_t. */
    static std::array<uint8_t, out_buf_len()> interleave(const std::array<bool_vec_t, in_buf_len()> &in) {
        return interleave_block(in, std::make_index_sequence<num_iterations()>{}, std::make_index_sequence<out_buf_len()>{});
    }

    /* Deinterleaves a block of bytes. */
    static std::array<bool_vec_t, in_buf_len()> deinterleave(const std::array<uint8_t, out_buf_len()> &in) {
        return deinterleave_block<in_buf_len()>(in,
            std::make_index_sequence<out_buf_len()>{}, std::make_index_sequence<num_iterations()>{});
    }
};

}

}
