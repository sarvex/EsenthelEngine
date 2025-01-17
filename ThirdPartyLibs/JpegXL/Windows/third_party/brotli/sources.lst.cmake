# IT WOULD BE FOOLISH TO USE COMPUTERS TO AUTOMATE REPETITIVE TASKS:
# ENLIST EVERY USED HEADER AND SOURCE FILE MANUALLY!

SET(BROTLI_CLI_C 
  c/tools/brotli.c)

SET(BROTLI_COMMON_C 
  c/common/dictionary.c 
  c/common/transform.c)

SET(BROTLI_COMMON_H 
  c/common/constants.h 
  c/common/context.h 
  c/common/dictionary.h 
  c/common/platform.h 
  c/common/transform.h 
  c/common/version.h)

SET(BROTLI_DEC_C 
  c/dec/bit_reader.c 
  c/dec/decode.c 
  c/dec/huffman.c 
  c/dec/state.c)

SET(BROTLI_DEC_H 
  c/dec/bit_reader.h 
  c/dec/huffman.h 
  c/dec/prefix.h 
  c/dec/state.h)

SET(BROTLI_ENC_C 
  c/enc/backward_references.c 
  c/enc/backward_references_hq.c 
  c/enc/bit_cost.c 
  c/enc/block_splitter.c 
  c/enc/brotli_bit_stream.c 
  c/enc/cluster.c 
  c/enc/compress_fragment.c 
  c/enc/compress_fragment_two_pass.c 
  c/enc/dictionary_hash.c 
  c/enc/encode.c 
  c/enc/encoder_dict.c 
  c/enc/entropy_encode.c 
  c/enc/histogram.c 
  c/enc/literal_cost.c 
  c/enc/memory.c 
  c/enc/metablock.c 
  c/enc/static_dict.c 
  c/enc/utf8_util.c)

SET(BROTLI_ENC_H 
  c/enc/backward_references.h 
  c/enc/backward_references_hq.h 
  c/enc/backward_references_inc.h 
  c/enc/bit_cost.h 
  c/enc/bit_cost_inc.h 
  c/enc/block_encoder_inc.h 
  c/enc/block_splitter.h 
  c/enc/block_splitter_inc.h 
  c/enc/brotli_bit_stream.h 
  c/enc/cluster.h 
  c/enc/cluster_inc.h 
  c/enc/command.h 
  c/enc/compress_fragment.h 
  c/enc/compress_fragment_two_pass.h 
  c/enc/dictionary_hash.h 
  c/enc/encoder_dict.h 
  c/enc/entropy_encode.h 
  c/enc/entropy_encode_static.h 
  c/enc/fast_log.h 
  c/enc/find_match_length.h 
  c/enc/hash.h 
  c/enc/hash_composite_inc.h 
  c/enc/hash_forgetful_chain_inc.h 
  c/enc/hash_longest_match64_inc.h 
  c/enc/hash_longest_match_inc.h 
  c/enc/hash_longest_match_quickly_inc.h 
  c/enc/hash_rolling_inc.h 
  c/enc/hash_to_binary_tree_inc.h 
  c/enc/histogram.h 
  c/enc/histogram_inc.h 
  c/enc/literal_cost.h 
  c/enc/memory.h 
  c/enc/metablock.h 
  c/enc/metablock_inc.h 
  c/enc/params.h 
  c/enc/prefix.h 
  c/enc/quality.h 
  c/enc/ringbuffer.h 
  c/enc/static_dict.h 
  c/enc/static_dict_lut.h 
  c/enc/utf8_util.h 
  c/enc/write_bits.h)

SET(BROTLI_INCLUDE 
  c/include/brotli/decode.h 
  c/include/brotli/encode.h 
  c/include/brotli/port.h 
  c/include/brotli/types.h)
