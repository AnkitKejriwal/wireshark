###################################################
# create C header files for an IDL structure
# Copyright tridge@samba.org 2000
# Copyright jelmer@samba.org 2005
# released under the GNU GPL

package Parse::Pidl::Samba::NDR::Header;

use strict;
use Parse::Pidl::Typelist qw(mapType);
use Parse::Pidl::Util qw(has_property is_constant);
use Parse::Pidl::NDR qw(GetNextLevel GetPrevLevel);
use Parse::Pidl::Samba::NDR::Parser;

use vars qw($VERSION);
$VERSION = '0.01';

my($res);
my($tab_depth);

sub pidl ($)
{
	$res .= shift;
}

sub tabs()
{
	my $res = "";
	$res .="\t" foreach (1..$tab_depth);
	return $res;
}

#####################################################################
# prototype a typedef
sub HeaderTypedefProto($)
{
    my($d) = shift;

	my $tf = Parse::Pidl::Samba::NDR::Parser::get_typefamily($d->{DATA}{TYPE});

    if (has_property($d, "gensize")) {
		my $size_args = $tf->{SIZE_FN_ARGS}->($d);
		pidl "size_t ndr_size_$d->{NAME}($size_args);\n";
    }

    return unless has_property($d, "public");

	unless (has_property($d, "nopush")) {
		pidl "NTSTATUS ndr_push_$d->{NAME}(struct ndr_push *ndr, int ndr_flags, " . $tf->{DECL}->($d, "push") . ");\n";
	}
	unless (has_property($d, "nopull")) {
	    pidl "NTSTATUS ndr_pull_$d->{NAME}(struct ndr_pull *ndr, int ndr_flags, " . $tf->{DECL}->($d, "pull") . ");\n";
	}
    unless (has_property($d, "noprint")) {
	    pidl "void ndr_print_$d->{NAME}(struct ndr_print *ndr, const char *name, " . $tf->{DECL}->($d, "print") . ");\n";
    }
}

#####################################################################
# output prototypes for a IDL function
sub HeaderFnProto($$)
{
	my ($interface,$fn) = @_;
	my $name = $fn->{NAME};

	pidl "void ndr_print_$name(struct ndr_print *ndr, const char *name, int flags, const struct $name *r);\n";

	unless (has_property($fn, "noopnum")) {
		pidl "NTSTATUS dcerpc_$name(struct dcerpc_pipe *p, TALLOC_CTX *mem_ctx, struct $name *r);\n";
   		pidl "struct rpc_request *dcerpc_$name\_send(struct dcerpc_pipe *p, TALLOC_CTX *mem_ctx, struct $name *r);\n";
	}

	return unless has_property($fn, "public");

	pidl "NTSTATUS ndr_push_$name(struct ndr_push *ndr, int flags, const struct $name *r);\n";
	pidl "NTSTATUS ndr_pull_$name(struct ndr_pull *ndr, int flags, struct $name *r);\n";

	pidl "\n";
}

#####################################################################
# parse the interface definitions
sub HeaderInterface($)
{
	my($interface) = shift;

	if (defined $interface->{PROPERTIES}->{depends}) {
		my @d = split / /, $interface->{PROPERTIES}->{depends};
		foreach my $i (@d) {
			pidl "#include \"librpc/gen_ndr/ndr_$i\.h\"\n";
		}
	}

	my $count = 0;

	pidl "#ifndef _HEADER_NDR_$interface->{NAME}\n";
	pidl "#define _HEADER_NDR_$interface->{NAME}\n\n";

	if (defined $interface->{PROPERTIES}->{uuid}) {
		my $name = uc $interface->{NAME};
		pidl "#define DCERPC_$name\_UUID " . 
		Parse::Pidl::Util::make_str($interface->{PROPERTIES}->{uuid}) . "\n";

		if(!defined $interface->{PROPERTIES}->{version}) { $interface->{PROPERTIES}->{version} = "0.0"; }
		pidl "#define DCERPC_$name\_VERSION $interface->{PROPERTIES}->{version}\n";

		pidl "#define DCERPC_$name\_NAME \"$interface->{NAME}\"\n";

		if(!defined $interface->{PROPERTIES}->{helpstring}) { $interface->{PROPERTIES}->{helpstring} = "NULL"; }
		pidl "#define DCERPC_$name\_HELPSTRING $interface->{PROPERTIES}->{helpstring}\n";

		pidl "\nextern const struct dcerpc_interface_table dcerpc_table_$interface->{NAME};\n";
		pidl "NTSTATUS dcerpc_server_$interface->{NAME}_init(void);\n\n";
	}

	foreach my $d (@{$interface->{DATA}}) {
		next if $d->{TYPE} ne "FUNCTION";
		next if has_property($d, "noopnum");
		next if grep(/$d->{NAME}/,@{$interface->{INHERITED_FUNCTIONS}});
		my $u_name = uc $d->{NAME};
		pidl "#define DCERPC_$u_name (";
	
		if (defined($interface->{BASE})) {
			pidl "DCERPC_" . uc $interface->{BASE} . "_CALL_COUNT + ";
		}

		pidl sprintf("0x%02x", $count) . ")\n";
		$count++;
	}

	pidl "\n#define DCERPC_" . uc $interface->{NAME} . "_CALL_COUNT (";
	
	if (defined($interface->{BASE})) {
		pidl "DCERPC_" . uc $interface->{BASE} . "_CALL_COUNT + ";
	}
	
	pidl "$count)\n\n";

	foreach my $d (@{$interface->{DATA}}) {
		next if ($d->{TYPE} ne "TYPEDEF");
		HeaderTypedefProto($d);
	}

	foreach my $d (@{$interface->{DATA}}) {
		next if ($d->{TYPE} ne "FUNCTION");
		HeaderFnProto($interface, $d);
	}

	pidl "#endif /* _HEADER_NDR_$interface->{NAME} */\n";
}

#####################################################################
# parse a parsed IDL into a C header
sub Parse($$)
{
    my($idl,$basename) = @_;
    $tab_depth = 0;

	$res = "";
    pidl "/* header auto-generated by pidl */\n";
	pidl "#include \"librpc/gen_ndr/$basename\.h\"\n\n";

    foreach my $x (@{$idl}) {
	    ($x->{TYPE} eq "INTERFACE") && HeaderInterface($x);
    }
    return $res;
}

1;
