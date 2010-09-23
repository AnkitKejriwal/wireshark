#!/usr/bin/perl -w
# create the sminmpec.c file from
# http://www.iana.org/assignments/enterprise-numbers
#
# $Id$
#
# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
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

$in = "http://www.iana.org/assignments/enterprise-numbers" unless(defined $in);

my @in_lines;

if($in =~ m/^http:/i) {
	eval "require LWP::UserAgent;";
	die "LWP isn't installed. It is part of the standard Perl module libwww." if $@;

	my $agent    = LWP::UserAgent->new;

	warn "starting to fetch $in ...\n";

	my $request  = HTTP::Request->new(GET => $in);


	if (-f "enterprise-numbers") {
		my $mtime;
		(undef,undef,undef,undef,undef,undef,undef,undef,undef,$mtime,undef,undef,undef) = stat("enterprise-numbers");
		$request->if_modified_since( $mtime );
	}

	my $result   = $agent->request($request);

	if ($result->code eq 200) {
		warn "done fetching $in\n";
		@in_lines = split /\n/, $result->content;
		open ENFILE, "> enterprise-numbers";

		for (@in_lines) {
			chomp;
			print ENFILE "$_\n";
		}

		close ENFILE;
	} elsif ($result->code eq 304) {
		warn "enterprise-numbers was up-to-date\n";
		open IN, "< enterprise-numbers";
		@in_lines = <IN>;
		close IN;
	} else {
		die "request for $in failed with result code:" . $result->code;
	}

} else {
  open IN, "< $in";
  @in_lines = <IN>;
  close IN;
}


open OUT, "> sminmpec.c";

my $body = '';
my $code;
my $prev_code = -1;  ## Assumption: First code in enterprise file is 0;

sub escape_non_ascii {
    my $val = unpack 'C', $_[0];
    return sprintf '\0%.3o',$val;
}


for(@in_lines) {
	s/[\000-\037]//g;
	s/\\/\\\\/g;
	s/"/\\"/g;
	s/([\x80-\xFF])/escape_non_ascii($1)/ge;

	if (/^(\d+)/) {
		$code = sprintf("%5d", $1);
	} if (/^  (\S.*)/ ) {
		my $name = $1;
		if ($code < $prev_code) {
			print STDERR ("Input 'Codes' not sorted in ascending order (or duplicate codes)): $prev_code $code\n");
                        exit 1;
		}
		while ($code > ($prev_code+1)) {
			$prev_code = sprintf("%5d", $prev_code+1);
			$body .= "    { $prev_code, sminmpec_unknown },  /* (Added by Wireshark) */\n";
		}
		$prev_code = $code;
		$body .= "    { $code, \"$name\" },\n";
	}
}

print OUT <<"_SMINMPEC";
/*
 * \$Id\$
 *
 * THIS FILE IS AUTOGENERATED, DO NOT EDIT
 * generated from http://www.iana.org/assignments/enterprise-numbers
 * run "tools/make-sminmspec <infile> <outfile>" to regenerate
 *
 * Note: "Gaps" in the iana enterprise-numbers list have been "filled in"
 *       with "(Unknown)" as the name so that direct (indexed) access
 *       to the list is possible.
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>

#include <epan/value_string.h>
#include <epan/sminmpec.h>

static const gchar sminmpec_unknown[] = "(Unknown)";

static const value_string sminmpec_values[] = {
$body    {    0, NULL}
};

#define array_length(x)	(sizeof x / sizeof x[0])

value_string_ext sminmpec_values_ext = VALUE_STRING_EXT_INIT(sminmpec_values);

_SMINMPEC

close OUT;
