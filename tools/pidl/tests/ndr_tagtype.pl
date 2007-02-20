#!/usr/bin/perl
# Support for tagged types
# (C) 2005 Jelmer Vernooij. Published under the GNU GPL
use strict;

use Test::More tests => 1 * 8;
use FindBin qw($RealBin);
use lib "$RealBin";
use Util qw(test_samba4_ndr);

test_samba4_ndr('struct-notypedef', '[public] struct bla { uint8 x; }; ',
'
	struct ndr_push *ndr = ndr_push_init_ctx(NULL);
	struct bla r;
	uint8_t expected[] = { 0x0D };
	DATA_BLOB expected_blob = { expected, 1 };
	DATA_BLOB result_blob;
	r.x = 13;

	if (NT_STATUS_IS_ERR(ndr_push_bla(ndr, NDR_SCALARS|NDR_BUFFERS, &r)))
		return 1;

	result_blob = ndr_push_blob(ndr);
	
	if (!data_blob_equal(&result_blob, &expected_blob)) 
		return 2;
');
