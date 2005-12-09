#!/usr/bin/perl -w
# create the sminmpec.c file from 
# http://www.iana.org/assignments/enterprise-numbers
#
# $Id$
#
# Ethereal - Network traffic analyzer
# By Gerald Combs <gerald@ethereal.com>
# Copyright 2004 Gerald Combs
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
use strict;

my $in = shift;
my $out = shift;

open IN, "< $in";
open OUT, "> $out";

my $body = '';
my $code;

sub escape_non_ascii {
    my $val = unpack 'C', $_[0];
    return sprintf '\0%.3o',$val;
}

while(<IN>) {
	s/[\000-\037]//g;
	s/\\/\\\\/g;
	s/"/\\"/g;
	s/([\x80-\xFF])/escape_non_ascii($1)/ge;
	
	if (/^(\d+)/) {
		$code = $1;
	} if (/^  (\S.*)/ ) {
		my $name = $1;
		$body .= "\t{ $code,\t\"$name\" },\n";
	}
}

close IN;

print OUT <<"_SMINMPEC";
/*
 * THIS FILE IS AUTOGENERATED, DO NOT EDIT
 * generated from http://www.iana.org/assignments/enterprise-numbers
 * run "epan/make-sminmspec <infile> <outfile>" to regenerate
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>

#include <epan/value_string.h>
#include <epan/sminmpec.h>

const value_string sminmpec_values[] = {

$body
	{0, NULL}
};

_SMINMPEC

close OUT;
