###################################################
# create C header files for an EJS mapping functions
# Copyright tridge@samba.org 2005
# released under the GNU GPL

package Parse::Pidl::Samba::EJSHeader;

use strict;
use Parse::Pidl::Typelist;
use Parse::Pidl::Util qw(has_property);

my($res);

sub pidl ($)
{
	$res .= shift;
}

#####################################################################
# prototype a typedef
sub HeaderTypedefProto($)
{
	my $d = shift;
	my $name = $d->{NAME};
	
	return unless has_property($d, "public");
	
	my $type_decl = Parse::Pidl::Typelist::mapType($name);

	pidl "NTSTATUS ejs_push_$d->{NAME}(struct ejs_rpc *, struct MprVar *, const char *, const $type_decl *);\n";
	pidl "NTSTATUS ejs_pull_$d->{NAME}(struct ejs_rpc *, struct MprVar *, const char *, $type_decl *);\n";
}

#####################################################################
# parse the interface definitions
sub HeaderInterface($)
{
	my($interface) = shift;

	my $count = 0;

	pidl "#ifndef _HEADER_EJS_$interface->{NAME}\n";
	pidl "#define _HEADER_EJS_$interface->{NAME}\n\n";

	if (defined $interface->{PROPERTIES}->{depends}) {
		my @d = split / /, $interface->{PROPERTIES}->{depends};
		foreach my $i (@d) {
			pidl "#include \"librpc/gen_ndr/ndr_$i\_ejs\.h\"\n";
		}
	}

	pidl "\n";

	foreach my $d (@{$interface->{TYPEDEFS}}) {
		HeaderTypedefProto($d);
	}

	pidl "\n";
	pidl "#endif /* _HEADER_EJS_$interface->{NAME} */\n";
}

#####################################################################
# parse a parsed IDL into a C header
sub Parse($)
{
    my($idl) = shift;

    $res = "";
    pidl "/* header auto-generated by pidl */\n\n";
    foreach my $x (@{$idl}) {
	    ($x->{TYPE} eq "INTERFACE") && HeaderInterface($x);
    }
    return $res;
}

1;
