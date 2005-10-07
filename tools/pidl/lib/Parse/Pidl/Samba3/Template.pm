###################################################
# Samba3 NDR client generator for IDL structures
# Copyright jelmer@samba.org 2005
# released under the GNU GPL

package Parse::Pidl::Samba3::Template;

use strict;
use Parse::Pidl::Typelist qw(hasType getType mapType);
use Parse::Pidl::Util qw(has_property ParseExpr);
use Parse::Pidl::NDR qw(GetPrevLevel GetNextLevel ContainsDeferred);

use vars qw($VERSION);
$VERSION = '0.01';

my $res;
sub pidl($) { my $x = shift; $res.="$x\n"; }

sub ParseInterface($)
{
	my $if = shift;

	foreach (@{$if->{FUNCTIONS}}) {
		my $ret = $_->{RETURN_TYPE};
		if (not $ret) { $ret = "void"; }
		pidl "$ret _$_->{NAME}(pipes_struct *p, " . uc($if->{NAME}) . "_Q_" . uc($_->{NAME}) . " *q_u, " . uc($if->{NAME}) . "_R_" . uc($_->{NAME}) . " *r_u)";
		pidl "{";
		pidl "\t/* FIXME: Implement your code here */";
		if (not defined($_->{RETURN_TYPE})) {
		} elsif ($_->{RETURN_TYPE} eq "WERROR") {
			pidl "\treturn WERR_NOT_SUPPORTED;";
		} elsif ($_->{RETURN_TYPE} eq "NTSTATUS") {
			pidl "\treturn NT_STATUS_NOT_IMPLEMENTED;";
		} elsif ($_->{RETURN_TYPE} eq "uint32") {
			pidl "\treturn 0;";
		}
		pidl "}";
		pidl "";
	}
}

sub Parse($$)
{
	my($ndr,$filename) = @_;

	$res = "";

	pidl "/*";
	pidl " * Unix SMB/CIFS implementation.";
	pidl " * template auto-generated by pidl. Modify to your needs";
	pidl " */";
	pidl "";
	pidl "#include \"includes.h\"";
	pidl "";
	pidl "#undef DBGC_CLASS";
	pidl "#define DBGC_CLASS DBGC_MSRPC";
	pidl "";
	
	foreach (@$ndr) {
		ParseInterface($_) if ($_->{TYPE} eq "INTERFACE");
	}

	return $res;
}

1;
