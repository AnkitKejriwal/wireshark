#!/usr/bin/perl
#
# make-taps.pl
# Extract structs from C headers to generate a function that
# pushes a lua table into the stack containing the elements of
# the struct.
#
# (c) 2006 Luis E. Garcia Ontanon <luis@ontanon.org>
#
# $Id$
#
# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
# Copyright 2006 Gerald Combs
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

use strict;

my %types = %{{
	'gchar[]' => 'lua_pushstring(L,(char*)v->%s);',
	'gchar*' => 'lua_pushstring(L,(char*)v->%s);',
	'guint' => 'lua_pushnumber(L,(lua_Number)v->%s);',
	'guint8' => 'lua_pushnumber(L,(lua_Number)v->%s);',
	'guint16' => 'lua_pushnumber(L,(lua_Number)v->%s);',
	'guint32' => 'lua_pushnumber(L,(lua_Number)v->%s);',
	'gint' => 'lua_pushnumber(L,(lua_Number)v->%s);',
	'gint8' => 'lua_pushnumber(L,(lua_Number)v->%s);',
	'gint16' => 'lua_pushnumber(L,(lua_Number)v->%s);',
	'gint32' => 'lua_pushnumber(L,(lua_Number)v->%s);',
	'gboolean' => 'lua_pushboolean(L,(int)v->%s);',
	'address' => '{ Address a = g_malloc(sizeof(address)); COPY_ADDRESS(a, &(v->%s)); pushAddress(L,a); }',
	'address*' => '{ Address a = g_malloc(sizeof(address)); COPY_ADDRESS(a, v->%s); pushAddress(L,a); }',
	'int' => 'lua_pushnumber(L,(lua_Number)v->%s);',
	'nstime_t' => '{lua_Number t = (lua_Number) v->%s.secs; t += v->%s.nsecs * 1e-9; lua_pushnumber(L,t); }',
	'nstime_t*' => '{lua_Number t = (lua_Number) v->%s->secs; t += v->%s->nsecs * 1e-9; lua_pushnumber(L,t); }',
}};

my %comments = %{{
	'gchar[]' => 'string',
	'gchar*' => 'string',
	'guint' => 'number',
	'guint8' => 'number',
	'guint16' => 'number',
	'guint32' => 'number',
	'gint' => 'number',
	'gint8' => 'number',
	'gint16' => 'number',
	'gint32' => 'number',
	'gboolean' => 'boolean',
	'address' => 'Address',
	'address*' => 'Address',
	'int' => 'number',
	'nstime_t' => 'number (seconds, since 1-1-1970 if absolute)',
	'nstime_t*' => 'number (seconds, since 1-1-1970 if absolute)',
}};


my %functs = ();

my %enums = ();

sub dotap {
	my ($tname,$fname,$sname,@enums) = @_;
	my $buf = '';
	
	open FILE, "< $fname";
	while(<FILE>) {
		$buf .= $_;
	}
	close FILE;
	
	$buf =~ s@/\*.*?\*/@@;

	for my $ename (@enums) {
		$enums{$ename} = [];
		my $a = $enums{$ename};
		
		my $enumre = "typedef\\s+enum[^{]*{([^}]*)}[\\s\\n]*" . ${ename} . "[\\s\\n]*;";
		if ($buf =~ s/$enumre//ms ) {
			$types{$ename} = "/*$ename*/ lua_pushnumber(L,(lua_Number)v->%s);";
			my $ebody = $1;
			$ebody =~ s/\s+//msg;
			$comments{$ename} = "$ename: { $ebody }";
			$comments{$ename} =~ s/,/|/g;
			for (split /,/, $ebody) {
				push @{$a}, $_;
			}
		}
	}

	my $re = "typedef\\s+struct.*?{([^}]*)}[\\s\\n]*($sname)[\\s\\n]*;";
	my $body;

	while ($buf =~ s/$re//ms) {
		   $body = $1;
	}

	die "could not find typedef $sname in $fname" if not defined $body and  $sname ne "void";

	my %elems = ();

	while($body =~ s/\s*(.*?)([\w\d_]+)\s*\[\s*\d+\s*\]\s*;//) {
		  my ($k,$v) = ($2,$1 . "[]");
		  $v =~ s/\s+//g;
		  $elems{$k} = $v;
	}

	while($body =~ s/\s*(.*?)([\w\d_]+)\s*;//) {
		  my ($k,$v) = ($2,$1);
		  $v =~ s/\s+//g;
		  $elems{$k} = $v;
	}

	my $code = "static void wslua_${tname}_to_table(lua_State* L, const void* p) { $sname* v = (void*)p; lua_newtable(L);\n";
	my $doc = "Tap: $tname\n";

	for my $n (sort keys %elems) {
		my $fmt = $types{$elems{$n}};
		
		if ($fmt) {
			$code .= "\tlua_pushstring(L,\"$n\"); ";
			$code .= sprintf($fmt,$n,$n) . " lua_settable(L,-3);\n";
			$doc .= "\t$n: $comments{$elems{$n}}\n";
		}
		
	}

	$code .= "}\n\n";
	$doc .= "\n";

	$functs{$tname} = "wslua_${tname}_to_table";

	return ($code,$doc);
}


open TAPSFILE, "< $ARGV[0]";
open CFILE, "> $ARGV[1]";
open DOCFILE, "> $ARGV[2]";

print CFILE  <<"HEADER";
/*  This file is autogenerated from ./taps by ./make-taps.pl */
/* DO NOT EDIT! */

#include "config.h"

#include "wslua.h"

HEADER

print DOCFILE "\n";

while (<TAPSFILE>) {
	s@#.*@@;
	next if /^\s*$/;
	my ($tname,$fname,$sname,@enums) = split /\s+/;
	my ($c,$doc) = dotap($tname,$fname,$sname,@enums);
	print CFILE "#include \"$fname\"\n";
	print CFILE $c;
	print DOCFILE $doc;
}

print CFILE  <<"TBLHDR";
static tappable_t tappables[] =  {
TBLHDR

	for my $tname (sort keys %functs) {
		print CFILE  <<"TBLELEM";
	{"$tname", $functs{$tname} },
TBLELEM
	}
	
print CFILE  <<"TBLFTR";
	{"frame",NULL},
	{NULL,NULL}
};

int wslua_set_tap_enums(lua_State* L _U_) {
TBLFTR


for my $ename (sort keys %enums) {
	print CFILE  "\n\t/*\n\t * $ename\n\t */\n\tlua_newtable(L);\n";
	for my $a (@{$enums{$ename}}) {
		print CFILE  <<"ENUMELEM";
		lua_pushnumber(L,(lua_Number)$a); lua_setglobal(L,"$a");
		lua_pushnumber(L,(lua_Number)$a); lua_pushstring(L,"$a"); lua_settable(L,-3);
ENUMELEM
	}
	print CFILE "\tlua_setglobal(L,\"$ename\");\n";
}

print CFILE <<"TAIL";
	return 0;
}


tap_extractor_t wslua_get_tap_extractor(const gchar* name) {
	tappable_t* t;
	for(t = tappables; t->name; t++ ) {
		if (g_str_equal(t->name,name)) return t->extractor;
	}
	
	return NULL;
}

TAIL
exit 0;



