function fec_test_params = get_fec_test_parameters
    fec_test_params.rs_viterbi_message_data_length = 79;
    fec_test_params.rs_viterbi_message_parity_length = 16;
    fec_test_params.polar_block_size = 2048;
    fec_test_params.polar_block_size_shortened = 1536;
    fec_test_params.polar_data_size = 632;
    fec_test_params.test_polar = true;
    fec_test_params.test_rs_viterbi = false;
    fec_test_params.num_sims = 10000;
end

