#include <gtest/gtest.h>
#include <cstdlib>
#include "FEC/Polar.h"

template <typename T, T... I>
constexpr std::array<T, sizeof...(I)> expand_sequence(std::integer_sequence<T, I...>) {
    return std::array<T, sizeof...(I)>{ I ... };
}

TEST(PolarCodeConstructionTest, CalculateBhattacharyyaBounds) {
    /*
    Calculated using support/polar/polar_consruction.m – note that they are
    one-based.
    */
    std::array<std::size_t, 512> ref_indices = {
         127,  128,  192,  224,  232,  236,  238,  239,  240,  244,  246,  247,  248,  250,  251,  252,
         253,  254,  255,  256,  320,  352,  360,  364,  366,  367,  368,  372,  374,  375,  376,  378,
         379,  380,  381,  382,  383,  384,  400,  408,  412,  414,  415,  416,  424,  426,  427,  428,
         429,  430,  431,  432,  434,  435,  436,  437,  438,  439,  440,  441,  442,  443,  444,  445,
         446,  447,  448,  452,  454,  455,  456,  458,  459,  460,  461,  462,  463,  464,  466,  467,
         468,  469,  470,  471,  472,  473,  474,  475,  476,  477,  478,  479,  480,  482,  483,  484,
         485,  486,  487,  488,  489,  490,  491,  492,  493,  494,  495,  496,  497,  498,  499,  500,
         501,  502,  503,  504,  505,  506,  507,  508,  509,  510,  511,  512,  544,  560,  568,  572,
         574,  575,  576,  592,  596,  598,  599,  600,  602,  603,  604,  605,  606,  607,  608,  612,
         614,  615,  616,  618,  619,  620,  621,  622,  623,  624,  626,  627,  628,  629,  630,  631,
         632,  633,  634,  635,  636,  637,  638,  639,  640,  648,  652,  654,  655,  656,  660,  662,
         663,  664,  666,  667,  668,  669,  670,  671,  672,  676,  678,  679,  680,  682,  683,  684,
         685,  686,  687,  688,  689,  690,  691,  692,  693,  694,  695,  696,  697,  698,  699,  700,
         701,  702,  703,  704,  708,  709,  710,  711,  712,  713,  714,  715,  716,  717,  718,  719,
         720,  721,  722,  723,  724,  725,  726,  727,  728,  729,  730,  731,  732,  733,  734,  735,
         736,  737,  738,  739,  740,  741,  742,  743,  744,  745,  746,  747,  748,  749,  750,  751,
         752,  753,  754,  755,  756,  757,  758,  759,  760,  761,  762,  763,  764,  765,  766,  767,
         768,  776,  780,  782,  783,  784,  788,  789,  790,  791,  792,  793,  794,  795,  796,  797,
         798,  799,  800,  802,  803,  804,  805,  806,  807,  808,  809,  810,  811,  812,  813,  814,
         815,  816,  817,  818,  819,  820,  821,  822,  823,  824,  825,  826,  827,  828,  829,  830,
         831,  832,  834,  835,  836,  837,  838,  839,  840,  841,  842,  843,  844,  845,  846,  847,
         848,  849,  850,  851,  852,  853,  854,  855,  856,  857,  858,  859,  860,  861,  862,  863,
         864,  865,  866,  867,  868,  869,  870,  871,  872,  873,  874,  875,  876,  877,  878,  879,
         880,  881,  882,  883,  884,  885,  886,  887,  888,  889,  890,  891,  892,  893,  894,  895,
         896,  898,  899,  900,  901,  902,  903,  904,  905,  906,  907,  908,  909,  910,  911,  912,
         913,  914,  915,  916,  917,  918,  919,  920,  921,  922,  923,  924,  925,  926,  927,  928,
         929,  930,  931,  932,  933,  934,  935,  936,  937,  938,  939,  940,  941,  942,  943,  944,
         945,  946,  947,  948,  949,  950,  951,  952,  953,  954,  955,  956,  957,  958,  959,  960,
         961,  962,  963,  964,  965,  966,  967,  968,  969,  970,  971,  972,  973,  974,  975,  976,
         977,  978,  979,  980,  981,  982,  983,  984,  985,  986,  987,  988,  989,  990,  991,  992,
         993,  994,  995,  996,  997,  998,  999, 1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008,
        1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016, 1017, 1018, 1019, 1020, 1021, 1022, 1023, 1024
    };

    std::array<std::size_t, 512> data_indices = expand_sequence(
        Thiemar::Polar::PolarCodeConstructor<1024, 512, -2>::data_index_sequence{});

    for (std::size_t i = 0u; i < data_indices.size(); i++) {
        EXPECT_EQ((int)ref_indices.data()[i] - 1u, (int)data_indices.data()[i]) << "Buffers differ at index " << i;
    }
}
