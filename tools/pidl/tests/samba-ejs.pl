#!/usr/bin/perl
# (C) 2007 Jelmer Vernooij <jelmer@samba.org>
# Published under the GNU General Public License
use strict;
use warnings;

use Test::More tests => 13;
use FindBin qw($RealBin);
use lib "$RealBin";
use Util;
use Parse::Pidl::Util qw(MyDumper);
use Parse::Pidl::Samba4::EJS qw(get_pointer_to get_value_of check_null_pointer
        $res $res_hdr fn_declare);

is("&foo", get_pointer_to("foo"));
is("&(&foo)", get_pointer_to(get_pointer_to("foo")));
is("*foo", get_pointer_to("**foo"));
is("foo", get_pointer_to("*foo"));

is("foo", get_value_of("&foo"));
is("*foo", get_value_of("foo"));
is("**foo", get_value_of("*foo"));

$res = "";
check_null_pointer("bla");
is($res, "");

$res = "";
check_null_pointer("*bla");
is($res, "if (bla == NULL) return NT_STATUS_INVALID_PARAMETER_MIX;\n");

$res = "";
$res_hdr = "";
fn_declare({ PROPERTIES => { public => 1 } }, "myproto(int x)");
is($res, "_PUBLIC_ myproto(int x)\n");
is($res_hdr, "myproto(int x);\n");

$res = "";
$res_hdr = "";
fn_declare({ PROPERTIES => {} }, "mybla(int foo)");
is($res, "static mybla(int foo)\n");
is($res_hdr, "");
