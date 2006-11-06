####################################################################
#
#    This file was generated using Parse::Yapp version 1.05.
#
#        Don't edit this file, use source file instead.
#
#             ANY CHANGE MADE HERE WILL BE LOST !
#
####################################################################
package Parse::Pidl::IDL;
use vars qw ( @ISA );
use strict;

@ISA= qw ( Parse::Yapp::Driver );
#Included Parse/Yapp/Driver.pm file----------------------------------------
{
#
# Module Parse::Yapp::Driver
#
# This module is part of the Parse::Yapp package available on your
# nearest CPAN
#
# Any use of this module in a standalone parser make the included
# text under the same copyright as the Parse::Yapp module itself.
#
# This notice should remain unchanged.
#
# (c) Copyright 1998-2001 Francois Desarmenien, all rights reserved.
# (see the pod text in Parse::Yapp module for use and distribution rights)
#

package Parse::Yapp::Driver;

require 5.004;

use strict;

use vars qw ( $VERSION $COMPATIBLE $FILENAME );

$VERSION = '1.05';
$COMPATIBLE = '0.07';
$FILENAME=__FILE__;

use Carp;

#Known parameters, all starting with YY (leading YY will be discarded)
my(%params)=(YYLEX => 'CODE', 'YYERROR' => 'CODE', YYVERSION => '',
			 YYRULES => 'ARRAY', YYSTATES => 'ARRAY', YYDEBUG => '');
#Mandatory parameters
my(@params)=('LEX','RULES','STATES');

sub new {
    my($class)=shift;
	my($errst,$nberr,$token,$value,$check,$dotpos);
    my($self)={ ERROR => \&_Error,
				ERRST => \$errst,
                NBERR => \$nberr,
				TOKEN => \$token,
				VALUE => \$value,
				DOTPOS => \$dotpos,
				STACK => [],
				DEBUG => 0,
				CHECK => \$check };

	_CheckParams( [], \%params, \@_, $self );

		exists($$self{VERSION})
	and	$$self{VERSION} < $COMPATIBLE
	and	croak "Yapp driver version $VERSION ".
			  "incompatible with version $$self{VERSION}:\n".
			  "Please recompile parser module.";

        ref($class)
    and $class=ref($class);

    bless($self,$class);
}

sub YYParse {
    my($self)=shift;
    my($retval);

	_CheckParams( \@params, \%params, \@_, $self );

	if($$self{DEBUG}) {
		_DBLoad();
		$retval = eval '$self->_DBParse()';#Do not create stab entry on compile
        $@ and die $@;
	}
	else {
		$retval = $self->_Parse();
	}
    $retval
}

sub YYData {
	my($self)=shift;

		exists($$self{USER})
	or	$$self{USER}={};

	$$self{USER};
	
}

sub YYErrok {
	my($self)=shift;

	${$$self{ERRST}}=0;
    undef;
}

sub YYNberr {
	my($self)=shift;

	${$$self{NBERR}};
}

sub YYRecovering {
	my($self)=shift;

	${$$self{ERRST}} != 0;
}

sub YYAbort {
	my($self)=shift;

	${$$self{CHECK}}='ABORT';
    undef;
}

sub YYAccept {
	my($self)=shift;

	${$$self{CHECK}}='ACCEPT';
    undef;
}

sub YYError {
	my($self)=shift;

	${$$self{CHECK}}='ERROR';
    undef;
}

sub YYSemval {
	my($self)=shift;
	my($index)= $_[0] - ${$$self{DOTPOS}} - 1;

		$index < 0
	and	-$index <= @{$$self{STACK}}
	and	return $$self{STACK}[$index][1];

	undef;	#Invalid index
}

sub YYCurtok {
	my($self)=shift;

        @_
    and ${$$self{TOKEN}}=$_[0];
    ${$$self{TOKEN}};
}

sub YYCurval {
	my($self)=shift;

        @_
    and ${$$self{VALUE}}=$_[0];
    ${$$self{VALUE}};
}

sub YYExpect {
    my($self)=shift;

    keys %{$self->{STATES}[$self->{STACK}[-1][0]]{ACTIONS}}
}

sub YYLexer {
    my($self)=shift;

	$$self{LEX};
}


#################
# Private stuff #
#################


sub _CheckParams {
	my($mandatory,$checklist,$inarray,$outhash)=@_;
	my($prm,$value);
	my($prmlst)={};

	while(($prm,$value)=splice(@$inarray,0,2)) {
        $prm=uc($prm);
			exists($$checklist{$prm})
		or	croak("Unknow parameter '$prm'");
			ref($value) eq $$checklist{$prm}
		or	croak("Invalid value for parameter '$prm'");
        $prm=unpack('@2A*',$prm);
		$$outhash{$prm}=$value;
	}
	for (@$mandatory) {
			exists($$outhash{$_})
		or	croak("Missing mandatory parameter '".lc($_)."'");
	}
}

sub _Error {
	print "Parse error.\n";
}

sub _DBLoad {
	{
		no strict 'refs';

			exists(${__PACKAGE__.'::'}{_DBParse})#Already loaded ?
		and	return;
	}
	my($fname)=__FILE__;
	my(@drv);
	open(DRV,"<$fname") or die "Report this as a BUG: Cannot open $fname";
	while(<DRV>) {
                	/^\s*sub\s+_Parse\s*{\s*$/ .. /^\s*}\s*#\s*_Parse\s*$/
        	and     do {
                	s/^#DBG>//;
                	push(@drv,$_);
        	}
	}
	close(DRV);

	$drv[0]=~s/_P/_DBP/;
	eval join('',@drv);
}

#Note that for loading debugging version of the driver,
#this file will be parsed from 'sub _Parse' up to '}#_Parse' inclusive.
#So, DO NOT remove comment at end of sub !!!
sub _Parse {
    my($self)=shift;

	my($rules,$states,$lex,$error)
     = @$self{ 'RULES', 'STATES', 'LEX', 'ERROR' };
	my($errstatus,$nberror,$token,$value,$stack,$check,$dotpos)
     = @$self{ 'ERRST', 'NBERR', 'TOKEN', 'VALUE', 'STACK', 'CHECK', 'DOTPOS' };

#DBG>	my($debug)=$$self{DEBUG};
#DBG>	my($dbgerror)=0;

#DBG>	my($ShowCurToken) = sub {
#DBG>		my($tok)='>';
#DBG>		for (split('',$$token)) {
#DBG>			$tok.=		(ord($_) < 32 or ord($_) > 126)
#DBG>					?	sprintf('<%02X>',ord($_))
#DBG>					:	$_;
#DBG>		}
#DBG>		$tok.='<';
#DBG>	};

	$$errstatus=0;
	$$nberror=0;
	($$token,$$value)=(undef,undef);
	@$stack=( [ 0, undef ] );
	$$check='';

    while(1) {
        my($actions,$act,$stateno);

        $stateno=$$stack[-1][0];
        $actions=$$states[$stateno];

#DBG>	print STDERR ('-' x 40),"\n";
#DBG>		$debug & 0x2
#DBG>	and	print STDERR "In state $stateno:\n";
#DBG>		$debug & 0x08
#DBG>	and	print STDERR "Stack:[".
#DBG>					 join(',',map { $$_[0] } @$stack).
#DBG>					 "]\n";


        if  (exists($$actions{ACTIONS})) {

				defined($$token)
            or	do {
				($$token,$$value)=&$lex($self);
#DBG>				$debug & 0x01
#DBG>			and	print STDERR "Need token. Got ".&$ShowCurToken."\n";
			};

            $act=   exists($$actions{ACTIONS}{$$token})
                    ?   $$actions{ACTIONS}{$$token}
                    :   exists($$actions{DEFAULT})
                        ?   $$actions{DEFAULT}
                        :   undef;
        }
        else {
            $act=$$actions{DEFAULT};
#DBG>			$debug & 0x01
#DBG>		and	print STDERR "Don't need token.\n";
        }

            defined($act)
        and do {

                $act > 0
            and do {        #shift

#DBG>				$debug & 0x04
#DBG>			and	print STDERR "Shift and go to state $act.\n";

					$$errstatus
				and	do {
					--$$errstatus;

#DBG>					$debug & 0x10
#DBG>				and	$dbgerror
#DBG>				and	$$errstatus == 0
#DBG>				and	do {
#DBG>					print STDERR "**End of Error recovery.\n";
#DBG>					$dbgerror=0;
#DBG>				};
				};


                push(@$stack,[ $act, $$value ]);

					$$token ne ''	#Don't eat the eof
				and	$$token=$$value=undef;
                next;
            };

            #reduce
            my($lhs,$len,$code,@sempar,$semval);
            ($lhs,$len,$code)=@{$$rules[-$act]};

#DBG>			$debug & 0x04
#DBG>		and	$act
#DBG>		and	print STDERR "Reduce using rule ".-$act." ($lhs,$len): ";

                $act
            or  $self->YYAccept();

            $$dotpos=$len;

                unpack('A1',$lhs) eq '@'    #In line rule
            and do {
                    $lhs =~ /^\@[0-9]+\-([0-9]+)$/
                or  die "In line rule name '$lhs' ill formed: ".
                        "report it as a BUG.\n";
                $$dotpos = $1;
            };

            @sempar =       $$dotpos
                        ?   map { $$_[1] } @$stack[ -$$dotpos .. -1 ]
                        :   ();

            $semval = $code ? &$code( $self, @sempar )
                            : @sempar ? $sempar[0] : undef;

            splice(@$stack,-$len,$len);

                $$check eq 'ACCEPT'
            and do {

#DBG>			$debug & 0x04
#DBG>		and	print STDERR "Accept.\n";

				return($semval);
			};

                $$check eq 'ABORT'
            and	do {

#DBG>			$debug & 0x04
#DBG>		and	print STDERR "Abort.\n";

				return(undef);

			};

#DBG>			$debug & 0x04
#DBG>		and	print STDERR "Back to state $$stack[-1][0], then ";

                $$check eq 'ERROR'
            or  do {
#DBG>				$debug & 0x04
#DBG>			and	print STDERR 
#DBG>				    "go to state $$states[$$stack[-1][0]]{GOTOS}{$lhs}.\n";

#DBG>				$debug & 0x10
#DBG>			and	$dbgerror
#DBG>			and	$$errstatus == 0
#DBG>			and	do {
#DBG>				print STDERR "**End of Error recovery.\n";
#DBG>				$dbgerror=0;
#DBG>			};

			    push(@$stack,
                     [ $$states[$$stack[-1][0]]{GOTOS}{$lhs}, $semval ]);
                $$check='';
                next;
            };

#DBG>			$debug & 0x04
#DBG>		and	print STDERR "Forced Error recovery.\n";

            $$check='';

        };

        #Error
            $$errstatus
        or   do {

            $$errstatus = 1;
            &$error($self);
                $$errstatus # if 0, then YYErrok has been called
            or  next;       # so continue parsing

#DBG>			$debug & 0x10
#DBG>		and	do {
#DBG>			print STDERR "**Entering Error recovery.\n";
#DBG>			++$dbgerror;
#DBG>		};

            ++$$nberror;

        };

			$$errstatus == 3	#The next token is not valid: discard it
		and	do {
				$$token eq ''	# End of input: no hope
			and	do {
#DBG>				$debug & 0x10
#DBG>			and	print STDERR "**At eof: aborting.\n";
				return(undef);
			};

#DBG>			$debug & 0x10
#DBG>		and	print STDERR "**Dicard invalid token ".&$ShowCurToken.".\n";

			$$token=$$value=undef;
		};

        $$errstatus=3;

		while(	  @$stack
			  and (		not exists($$states[$$stack[-1][0]]{ACTIONS})
			        or  not exists($$states[$$stack[-1][0]]{ACTIONS}{error})
					or	$$states[$$stack[-1][0]]{ACTIONS}{error} <= 0)) {

#DBG>			$debug & 0x10
#DBG>		and	print STDERR "**Pop state $$stack[-1][0].\n";

			pop(@$stack);
		}

			@$stack
		or	do {

#DBG>			$debug & 0x10
#DBG>		and	print STDERR "**No state left on stack: aborting.\n";

			return(undef);
		};

		#shift the error token

#DBG>			$debug & 0x10
#DBG>		and	print STDERR "**Shift \$error token and go to state ".
#DBG>						 $$states[$$stack[-1][0]]{ACTIONS}{error}.
#DBG>						 ".\n";

		push(@$stack, [ $$states[$$stack[-1][0]]{ACTIONS}{error}, undef ]);

    }

    #never reached
	croak("Error in driver logic. Please, report it as a BUG");

}#_Parse
#DO NOT remove comment

1;

}
#End of include--------------------------------------------------




sub new {
        my($class)=shift;
        ref($class)
    and $class=ref($class);

    my($self)=$class->SUPER::new( yyversion => '1.05',
                                  yystates =>
[
	{#State 0
		DEFAULT => -1,
		GOTOS => {
			'idl' => 1
		}
	},
	{#State 1
		ACTIONS => {
			'' => 2
		},
		DEFAULT => -84,
		GOTOS => {
			'interface' => 3,
			'coclass' => 4,
			'property_list' => 5
		}
	},
	{#State 2
		DEFAULT => 0
	},
	{#State 3
		DEFAULT => -2
	},
	{#State 4
		DEFAULT => -3
	},
	{#State 5
		ACTIONS => {
			"coclass" => 6,
			"interface" => 8,
			"[" => 7
		}
	},
	{#State 6
		ACTIONS => {
			'IDENTIFIER' => 9
		},
		GOTOS => {
			'identifier' => 10
		}
	},
	{#State 7
		ACTIONS => {
			'IDENTIFIER' => 9
		},
		GOTOS => {
			'identifier' => 11,
			'properties' => 13,
			'property' => 12
		}
	},
	{#State 8
		ACTIONS => {
			'IDENTIFIER' => 9
		},
		GOTOS => {
			'identifier' => 14
		}
	},
	{#State 9
		DEFAULT => -113
	},
	{#State 10
		ACTIONS => {
			"{" => 15
		}
	},
	{#State 11
		ACTIONS => {
			"(" => 16
		},
		DEFAULT => -88
	},
	{#State 12
		DEFAULT => -86
	},
	{#State 13
		ACTIONS => {
			"," => 17,
			"]" => 18
		}
	},
	{#State 14
		ACTIONS => {
			":" => 19
		},
		DEFAULT => -8,
		GOTOS => {
			'base_interface' => 20
		}
	},
	{#State 15
		DEFAULT => -5,
		GOTOS => {
			'interface_names' => 21
		}
	},
	{#State 16
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'listtext' => 26,
			'anytext' => 25,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 17
		ACTIONS => {
			'IDENTIFIER' => 9
		},
		GOTOS => {
			'identifier' => 11,
			'property' => 29
		}
	},
	{#State 18
		DEFAULT => -85
	},
	{#State 19
		ACTIONS => {
			'IDENTIFIER' => 9
		},
		GOTOS => {
			'identifier' => 30
		}
	},
	{#State 20
		ACTIONS => {
			"{" => 31
		}
	},
	{#State 21
		ACTIONS => {
			"}" => 32,
			"interface" => 33
		}
	},
	{#State 22
		DEFAULT => -117
	},
	{#State 23
		DEFAULT => -95
	},
	{#State 24
		DEFAULT => -97
	},
	{#State 25
		ACTIONS => {
			"-" => 35,
			":" => 34,
			"<" => 37,
			"+" => 39,
			"~" => 38,
			"*" => 46,
			"?" => 36,
			"{" => 40,
			"&" => 41,
			"/" => 42,
			"=" => 43,
			"(" => 44,
			"|" => 45,
			"." => 47,
			">" => 48
		},
		DEFAULT => -90
	},
	{#State 26
		ACTIONS => {
			"," => 49,
			")" => 50
		}
	},
	{#State 27
		DEFAULT => -96
	},
	{#State 28
		DEFAULT => -116
	},
	{#State 29
		DEFAULT => -87
	},
	{#State 30
		DEFAULT => -9
	},
	{#State 31
		ACTIONS => {
			"typedef" => 51,
			"union" => 52,
			"enum" => 65,
			"bitmap" => 66,
			"declare" => 58,
			"const" => 60,
			"struct" => 63
		},
		DEFAULT => -84,
		GOTOS => {
			'typedecl' => 64,
			'function' => 53,
			'bitmap' => 67,
			'definitions' => 54,
			'definition' => 57,
			'property_list' => 56,
			'usertype' => 55,
			'declare' => 69,
			'const' => 68,
			'struct' => 59,
			'enum' => 61,
			'typedef' => 62,
			'union' => 70
		}
	},
	{#State 32
		ACTIONS => {
			";" => 71
		},
		DEFAULT => -118,
		GOTOS => {
			'optional_semicolon' => 72
		}
	},
	{#State 33
		ACTIONS => {
			'IDENTIFIER' => 9
		},
		GOTOS => {
			'identifier' => 73
		}
	},
	{#State 34
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 74,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 35
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 75,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 36
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 76,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 37
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 77,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 38
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 78,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 39
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 79,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 40
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 80,
			'text' => 24,
			'constant' => 27,
			'commalisttext' => 81
		}
	},
	{#State 41
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 82,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 42
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 83,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 43
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 84,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 44
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 80,
			'text' => 24,
			'constant' => 27,
			'commalisttext' => 85
		}
	},
	{#State 45
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 86,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 46
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 87,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 47
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 88,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 48
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 89,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 49
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 90,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 50
		DEFAULT => -89
	},
	{#State 51
		DEFAULT => -84,
		GOTOS => {
			'property_list' => 91
		}
	},
	{#State 52
		ACTIONS => {
			'IDENTIFIER' => 92
		},
		DEFAULT => -115,
		GOTOS => {
			'optional_identifier' => 93
		}
	},
	{#State 53
		DEFAULT => -12
	},
	{#State 54
		ACTIONS => {
			"}" => 94,
			"typedef" => 51,
			"union" => 52,
			"enum" => 65,
			"bitmap" => 66,
			"declare" => 58,
			"const" => 60,
			"struct" => 63
		},
		DEFAULT => -84,
		GOTOS => {
			'typedecl' => 64,
			'function' => 53,
			'bitmap' => 67,
			'definition' => 95,
			'property_list' => 56,
			'usertype' => 55,
			'const' => 68,
			'struct' => 59,
			'declare' => 69,
			'enum' => 61,
			'typedef' => 62,
			'union' => 70
		}
	},
	{#State 55
		ACTIONS => {
			";" => 96
		}
	},
	{#State 56
		ACTIONS => {
			'IDENTIFIER' => 9,
			"signed" => 102,
			"union" => 52,
			"enum" => 65,
			"bitmap" => 66,
			'void' => 97,
			"unsigned" => 103,
			"[" => 7,
			"struct" => 63
		},
		GOTOS => {
			'existingtype' => 101,
			'bitmap' => 67,
			'usertype' => 98,
			'identifier' => 99,
			'struct' => 59,
			'enum' => 61,
			'type' => 104,
			'union' => 70,
			'sign' => 100
		}
	},
	{#State 57
		DEFAULT => -10
	},
	{#State 58
		DEFAULT => -84,
		GOTOS => {
			'property_list' => 105
		}
	},
	{#State 59
		DEFAULT => -28
	},
	{#State 60
		ACTIONS => {
			'IDENTIFIER' => 9
		},
		GOTOS => {
			'identifier' => 106
		}
	},
	{#State 61
		DEFAULT => -30
	},
	{#State 62
		DEFAULT => -14
	},
	{#State 63
		ACTIONS => {
			'IDENTIFIER' => 92
		},
		DEFAULT => -115,
		GOTOS => {
			'optional_identifier' => 107
		}
	},
	{#State 64
		DEFAULT => -16
	},
	{#State 65
		ACTIONS => {
			'IDENTIFIER' => 92
		},
		DEFAULT => -115,
		GOTOS => {
			'optional_identifier' => 108
		}
	},
	{#State 66
		ACTIONS => {
			'IDENTIFIER' => 92
		},
		DEFAULT => -115,
		GOTOS => {
			'optional_identifier' => 109
		}
	},
	{#State 67
		DEFAULT => -31
	},
	{#State 68
		DEFAULT => -13
	},
	{#State 69
		DEFAULT => -15
	},
	{#State 70
		DEFAULT => -29
	},
	{#State 71
		DEFAULT => -119
	},
	{#State 72
		DEFAULT => -4
	},
	{#State 73
		ACTIONS => {
			";" => 110
		}
	},
	{#State 74
		ACTIONS => {
			"-" => 35,
			":" => 34,
			"<" => 37,
			"+" => 39,
			"~" => 38,
			"*" => 46,
			"?" => 36,
			"{" => 40,
			"&" => 41,
			"/" => 42,
			"=" => 43,
			"(" => 44,
			"|" => 45,
			"." => 47,
			">" => 48
		},
		DEFAULT => -107
	},
	{#State 75
		ACTIONS => {
			":" => 34,
			"<" => 37,
			"~" => 38,
			"?" => 36,
			"{" => 40,
			"=" => 43
		},
		DEFAULT => -98
	},
	{#State 76
		ACTIONS => {
			"-" => 35,
			":" => 34,
			"<" => 37,
			"+" => 39,
			"~" => 38,
			"*" => 46,
			"?" => 36,
			"{" => 40,
			"&" => 41,
			"/" => 42,
			"=" => 43,
			"(" => 44,
			"|" => 45,
			"." => 47,
			">" => 48
		},
		DEFAULT => -106
	},
	{#State 77
		ACTIONS => {
			"-" => 35,
			":" => 34,
			"<" => 37,
			"+" => 39,
			"~" => 38,
			"*" => 46,
			"?" => 36,
			"{" => 40,
			"&" => 41,
			"/" => 42,
			"=" => 43,
			"(" => 44,
			"|" => 45,
			"." => 47,
			">" => 48
		},
		DEFAULT => -102
	},
	{#State 78
		ACTIONS => {
			"-" => 35,
			":" => 34,
			"<" => 37,
			"+" => 39,
			"~" => 38,
			"*" => 46,
			"?" => 36,
			"{" => 40,
			"&" => 41,
			"/" => 42,
			"=" => 43,
			"(" => 44,
			"|" => 45,
			"." => 47,
			">" => 48
		},
		DEFAULT => -110
	},
	{#State 79
		ACTIONS => {
			":" => 34,
			"<" => 37,
			"~" => 38,
			"?" => 36,
			"{" => 40,
			"=" => 43
		},
		DEFAULT => -109
	},
	{#State 80
		ACTIONS => {
			"-" => 35,
			":" => 34,
			"<" => 37,
			"+" => 39,
			"~" => 38,
			"*" => 46,
			"?" => 36,
			"{" => 40,
			"&" => 41,
			"/" => 42,
			"=" => 43,
			"(" => 44,
			"|" => 45,
			"." => 47,
			">" => 48
		},
		DEFAULT => -92
	},
	{#State 81
		ACTIONS => {
			"}" => 111,
			"," => 112
		}
	},
	{#State 82
		ACTIONS => {
			":" => 34,
			"<" => 37,
			"~" => 38,
			"?" => 36,
			"{" => 40,
			"=" => 43
		},
		DEFAULT => -104
	},
	{#State 83
		ACTIONS => {
			":" => 34,
			"<" => 37,
			"~" => 38,
			"?" => 36,
			"{" => 40,
			"=" => 43
		},
		DEFAULT => -105
	},
	{#State 84
		ACTIONS => {
			"-" => 35,
			":" => 34,
			"<" => 37,
			"+" => 39,
			"~" => 38,
			"*" => 46,
			"?" => 36,
			"{" => 40,
			"&" => 41,
			"/" => 42,
			"=" => 43,
			"(" => 44,
			"|" => 45,
			"." => 47,
			">" => 48
		},
		DEFAULT => -108
	},
	{#State 85
		ACTIONS => {
			"," => 112,
			")" => 113
		}
	},
	{#State 86
		ACTIONS => {
			":" => 34,
			"<" => 37,
			"~" => 38,
			"?" => 36,
			"{" => 40,
			"=" => 43
		},
		DEFAULT => -103
	},
	{#State 87
		ACTIONS => {
			":" => 34,
			"<" => 37,
			"~" => 38,
			"?" => 36,
			"{" => 40,
			"=" => 43
		},
		DEFAULT => -100
	},
	{#State 88
		ACTIONS => {
			":" => 34,
			"<" => 37,
			"~" => 38,
			"?" => 36,
			"{" => 40,
			"=" => 43
		},
		DEFAULT => -99
	},
	{#State 89
		ACTIONS => {
			":" => 34,
			"<" => 37,
			"~" => 38,
			"?" => 36,
			"{" => 40,
			"=" => 43
		},
		DEFAULT => -101
	},
	{#State 90
		ACTIONS => {
			"-" => 35,
			":" => 34,
			"<" => 37,
			"+" => 39,
			"~" => 38,
			"*" => 46,
			"?" => 36,
			"{" => 40,
			"&" => 41,
			"/" => 42,
			"=" => 43,
			"(" => 44,
			"|" => 45,
			"." => 47,
			">" => 48
		},
		DEFAULT => -91
	},
	{#State 91
		ACTIONS => {
			'IDENTIFIER' => 9,
			"signed" => 102,
			"union" => 52,
			"enum" => 65,
			"bitmap" => 66,
			'void' => 97,
			"unsigned" => 103,
			"[" => 7,
			"struct" => 63
		},
		GOTOS => {
			'existingtype' => 101,
			'bitmap' => 67,
			'usertype' => 98,
			'identifier' => 99,
			'struct' => 59,
			'enum' => 61,
			'type' => 114,
			'union' => 70,
			'sign' => 100
		}
	},
	{#State 92
		DEFAULT => -114
	},
	{#State 93
		ACTIONS => {
			"{" => 116
		},
		DEFAULT => -69,
		GOTOS => {
			'union_body' => 117,
			'opt_union_body' => 115
		}
	},
	{#State 94
		ACTIONS => {
			";" => 71
		},
		DEFAULT => -118,
		GOTOS => {
			'optional_semicolon' => 118
		}
	},
	{#State 95
		DEFAULT => -11
	},
	{#State 96
		DEFAULT => -32
	},
	{#State 97
		DEFAULT => -40
	},
	{#State 98
		DEFAULT => -38
	},
	{#State 99
		DEFAULT => -37
	},
	{#State 100
		ACTIONS => {
			'IDENTIFIER' => 9
		},
		GOTOS => {
			'identifier' => 119
		}
	},
	{#State 101
		DEFAULT => -39
	},
	{#State 102
		DEFAULT => -33
	},
	{#State 103
		DEFAULT => -34
	},
	{#State 104
		ACTIONS => {
			'IDENTIFIER' => 9
		},
		GOTOS => {
			'identifier' => 120
		}
	},
	{#State 105
		ACTIONS => {
			"union" => 121,
			"enum" => 126,
			"bitmap" => 127,
			"[" => 7
		},
		GOTOS => {
			'decl_enum' => 122,
			'decl_bitmap' => 123,
			'decl_type' => 125,
			'decl_union' => 124
		}
	},
	{#State 106
		DEFAULT => -73,
		GOTOS => {
			'pointers' => 128
		}
	},
	{#State 107
		ACTIONS => {
			"{" => 130
		},
		DEFAULT => -59,
		GOTOS => {
			'struct_body' => 129,
			'opt_struct_body' => 131
		}
	},
	{#State 108
		ACTIONS => {
			"{" => 132
		},
		DEFAULT => -42,
		GOTOS => {
			'opt_enum_body' => 134,
			'enum_body' => 133
		}
	},
	{#State 109
		ACTIONS => {
			"{" => 136
		},
		DEFAULT => -50,
		GOTOS => {
			'bitmap_body' => 137,
			'opt_bitmap_body' => 135
		}
	},
	{#State 110
		DEFAULT => -6
	},
	{#State 111
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 138,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 112
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 139,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 113
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 140,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 114
		ACTIONS => {
			'IDENTIFIER' => 9
		},
		GOTOS => {
			'identifier' => 141
		}
	},
	{#State 115
		DEFAULT => -71
	},
	{#State 116
		DEFAULT => -66,
		GOTOS => {
			'union_elements' => 142
		}
	},
	{#State 117
		DEFAULT => -70
	},
	{#State 118
		DEFAULT => -7
	},
	{#State 119
		DEFAULT => -36
	},
	{#State 120
		ACTIONS => {
			"(" => 143
		}
	},
	{#State 121
		DEFAULT => -26
	},
	{#State 122
		DEFAULT => -21
	},
	{#State 123
		DEFAULT => -22
	},
	{#State 124
		DEFAULT => -23
	},
	{#State 125
		ACTIONS => {
			'IDENTIFIER' => 9
		},
		GOTOS => {
			'identifier' => 144
		}
	},
	{#State 126
		DEFAULT => -24
	},
	{#State 127
		DEFAULT => -25
	},
	{#State 128
		ACTIONS => {
			'IDENTIFIER' => 9,
			"*" => 146
		},
		GOTOS => {
			'identifier' => 145
		}
	},
	{#State 129
		DEFAULT => -60
	},
	{#State 130
		DEFAULT => -75,
		GOTOS => {
			'element_list1' => 147
		}
	},
	{#State 131
		DEFAULT => -61
	},
	{#State 132
		ACTIONS => {
			'IDENTIFIER' => 9
		},
		GOTOS => {
			'identifier' => 148,
			'enum_element' => 149,
			'enum_elements' => 150
		}
	},
	{#State 133
		DEFAULT => -43
	},
	{#State 134
		DEFAULT => -44
	},
	{#State 135
		DEFAULT => -52
	},
	{#State 136
		ACTIONS => {
			'IDENTIFIER' => 9
		},
		DEFAULT => -55,
		GOTOS => {
			'identifier' => 153,
			'bitmap_element' => 152,
			'bitmap_elements' => 151,
			'opt_bitmap_elements' => 154
		}
	},
	{#State 137
		DEFAULT => -51
	},
	{#State 138
		ACTIONS => {
			"-" => 35,
			":" => 34,
			"<" => 37,
			"+" => 39,
			"~" => 38,
			"*" => 46,
			"?" => 36,
			"{" => 40,
			"&" => 41,
			"/" => 42,
			"=" => 43,
			"(" => 44,
			"|" => 45,
			"." => 47,
			">" => 48
		},
		DEFAULT => -112
	},
	{#State 139
		ACTIONS => {
			"-" => 35,
			":" => 34,
			"<" => 37,
			"+" => 39,
			"~" => 38,
			"*" => 46,
			"?" => 36,
			"{" => 40,
			"&" => 41,
			"/" => 42,
			"=" => 43,
			"(" => 44,
			"|" => 45,
			"." => 47,
			">" => 48
		},
		DEFAULT => -93
	},
	{#State 140
		ACTIONS => {
			":" => 34,
			"<" => 37,
			"~" => 38,
			"?" => 36,
			"{" => 40,
			"=" => 43
		},
		DEFAULT => -111
	},
	{#State 141
		ACTIONS => {
			"[" => 155
		},
		DEFAULT => -81,
		GOTOS => {
			'array_len' => 156
		}
	},
	{#State 142
		ACTIONS => {
			"}" => 157
		},
		DEFAULT => -84,
		GOTOS => {
			'optional_base_element' => 159,
			'property_list' => 158
		}
	},
	{#State 143
		ACTIONS => {
			"," => -77,
			"void" => 163,
			")" => -77
		},
		DEFAULT => -84,
		GOTOS => {
			'base_element' => 160,
			'element_list2' => 162,
			'property_list' => 161
		}
	},
	{#State 144
		ACTIONS => {
			";" => 164
		}
	},
	{#State 145
		ACTIONS => {
			"[" => 155,
			"=" => 166
		},
		GOTOS => {
			'array_len' => 165
		}
	},
	{#State 146
		DEFAULT => -74
	},
	{#State 147
		ACTIONS => {
			"}" => 167
		},
		DEFAULT => -84,
		GOTOS => {
			'base_element' => 168,
			'property_list' => 161
		}
	},
	{#State 148
		ACTIONS => {
			"=" => 169
		},
		DEFAULT => -47
	},
	{#State 149
		DEFAULT => -45
	},
	{#State 150
		ACTIONS => {
			"}" => 170,
			"," => 171
		}
	},
	{#State 151
		ACTIONS => {
			"," => 172
		},
		DEFAULT => -56
	},
	{#State 152
		DEFAULT => -53
	},
	{#State 153
		ACTIONS => {
			"=" => 173
		}
	},
	{#State 154
		ACTIONS => {
			"}" => 174
		}
	},
	{#State 155
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			"]" => 175,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 176,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 156
		ACTIONS => {
			";" => 177
		}
	},
	{#State 157
		DEFAULT => -68
	},
	{#State 158
		ACTIONS => {
			"[" => 7
		},
		DEFAULT => -84,
		GOTOS => {
			'base_or_empty' => 178,
			'base_element' => 179,
			'empty_element' => 180,
			'property_list' => 181
		}
	},
	{#State 159
		DEFAULT => -67
	},
	{#State 160
		DEFAULT => -79
	},
	{#State 161
		ACTIONS => {
			'IDENTIFIER' => 9,
			"signed" => 102,
			"union" => 52,
			"enum" => 65,
			"bitmap" => 66,
			'void' => 97,
			"unsigned" => 103,
			"[" => 7,
			"struct" => 63
		},
		DEFAULT => -35,
		GOTOS => {
			'existingtype' => 101,
			'bitmap' => 67,
			'usertype' => 98,
			'identifier' => 99,
			'struct' => 59,
			'enum' => 61,
			'type' => 182,
			'union' => 70,
			'sign' => 100
		}
	},
	{#State 162
		ACTIONS => {
			"," => 183,
			")" => 184
		}
	},
	{#State 163
		DEFAULT => -78
	},
	{#State 164
		DEFAULT => -20
	},
	{#State 165
		ACTIONS => {
			"=" => 185
		}
	},
	{#State 166
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 186,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 167
		DEFAULT => -58
	},
	{#State 168
		ACTIONS => {
			";" => 187
		}
	},
	{#State 169
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 188,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 170
		DEFAULT => -41
	},
	{#State 171
		ACTIONS => {
			'IDENTIFIER' => 9
		},
		GOTOS => {
			'identifier' => 148,
			'enum_element' => 189
		}
	},
	{#State 172
		ACTIONS => {
			'IDENTIFIER' => 9
		},
		GOTOS => {
			'identifier' => 153,
			'bitmap_element' => 190
		}
	},
	{#State 173
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 191,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 174
		DEFAULT => -49
	},
	{#State 175
		ACTIONS => {
			"[" => 155
		},
		DEFAULT => -81,
		GOTOS => {
			'array_len' => 192
		}
	},
	{#State 176
		ACTIONS => {
			"-" => 35,
			":" => 34,
			"?" => 36,
			"<" => 37,
			"+" => 39,
			"~" => 38,
			"&" => 41,
			"{" => 40,
			"/" => 42,
			"=" => 43,
			"|" => 45,
			"(" => 44,
			"*" => 46,
			"." => 47,
			"]" => 193,
			">" => 48
		}
	},
	{#State 177
		DEFAULT => -27
	},
	{#State 178
		DEFAULT => -65
	},
	{#State 179
		ACTIONS => {
			";" => 194
		}
	},
	{#State 180
		DEFAULT => -64
	},
	{#State 181
		ACTIONS => {
			'IDENTIFIER' => 9,
			"signed" => 102,
			"union" => 52,
			";" => 195,
			"enum" => 65,
			"bitmap" => 66,
			'void' => 97,
			"unsigned" => 103,
			"[" => 7,
			"struct" => 63
		},
		DEFAULT => -35,
		GOTOS => {
			'existingtype' => 101,
			'bitmap' => 67,
			'usertype' => 98,
			'identifier' => 99,
			'struct' => 59,
			'enum' => 61,
			'type' => 182,
			'union' => 70,
			'sign' => 100
		}
	},
	{#State 182
		DEFAULT => -73,
		GOTOS => {
			'pointers' => 196
		}
	},
	{#State 183
		DEFAULT => -84,
		GOTOS => {
			'base_element' => 197,
			'property_list' => 161
		}
	},
	{#State 184
		ACTIONS => {
			";" => 198
		}
	},
	{#State 185
		ACTIONS => {
			'CONSTANT' => 28,
			'TEXT' => 22,
			'IDENTIFIER' => 9
		},
		DEFAULT => -94,
		GOTOS => {
			'identifier' => 23,
			'anytext' => 199,
			'text' => 24,
			'constant' => 27
		}
	},
	{#State 186
		ACTIONS => {
			"-" => 35,
			":" => 34,
			"?" => 36,
			"<" => 37,
			";" => 200,
			"+" => 39,
			"~" => 38,
			"&" => 41,
			"{" => 40,
			"/" => 42,
			"=" => 43,
			"|" => 45,
			"(" => 44,
			"*" => 46,
			"." => 47,
			">" => 48
		}
	},
	{#State 187
		DEFAULT => -76
	},
	{#State 188
		ACTIONS => {
			"-" => 35,
			":" => 34,
			"<" => 37,
			"+" => 39,
			"~" => 38,
			"*" => 46,
			"?" => 36,
			"{" => 40,
			"&" => 41,
			"/" => 42,
			"=" => 43,
			"(" => 44,
			"|" => 45,
			"." => 47,
			">" => 48
		},
		DEFAULT => -48
	},
	{#State 189
		DEFAULT => -46
	},
	{#State 190
		DEFAULT => -54
	},
	{#State 191
		ACTIONS => {
			"-" => 35,
			":" => 34,
			"<" => 37,
			"+" => 39,
			"~" => 38,
			"*" => 46,
			"?" => 36,
			"{" => 40,
			"&" => 41,
			"/" => 42,
			"=" => 43,
			"(" => 44,
			"|" => 45,
			"." => 47,
			">" => 48
		},
		DEFAULT => -57
	},
	{#State 192
		DEFAULT => -82
	},
	{#State 193
		ACTIONS => {
			"[" => 155
		},
		DEFAULT => -81,
		GOTOS => {
			'array_len' => 201
		}
	},
	{#State 194
		DEFAULT => -63
	},
	{#State 195
		DEFAULT => -62
	},
	{#State 196
		ACTIONS => {
			'IDENTIFIER' => 9,
			"*" => 146
		},
		GOTOS => {
			'identifier' => 202
		}
	},
	{#State 197
		DEFAULT => -80
	},
	{#State 198
		DEFAULT => -19
	},
	{#State 199
		ACTIONS => {
			"-" => 35,
			":" => 34,
			"?" => 36,
			"<" => 37,
			";" => 203,
			"+" => 39,
			"~" => 38,
			"&" => 41,
			"{" => 40,
			"/" => 42,
			"=" => 43,
			"|" => 45,
			"(" => 44,
			"*" => 46,
			"." => 47,
			">" => 48
		}
	},
	{#State 200
		DEFAULT => -17
	},
	{#State 201
		DEFAULT => -83
	},
	{#State 202
		ACTIONS => {
			"[" => 155
		},
		DEFAULT => -81,
		GOTOS => {
			'array_len' => 204
		}
	},
	{#State 203
		DEFAULT => -18
	},
	{#State 204
		DEFAULT => -72
	}
],
                                  yyrules  =>
[
	[#Rule 0
		 '$start', 2, undef
	],
	[#Rule 1
		 'idl', 0, undef
	],
	[#Rule 2
		 'idl', 2,
sub
#line 19 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 3
		 'idl', 2,
sub
#line 20 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 4
		 'coclass', 7,
sub
#line 24 "idl.yp"
{$_[3] => {
               "TYPE" => "COCLASS", 
	       "PROPERTIES" => $_[1],
	       "NAME" => $_[3],
	       "DATA" => $_[5],
		   "FILE" => $_[0]->YYData->{INPUT_FILENAME},
		   "LINE" => $_[0]->YYData->{LINE},
          }}
	],
	[#Rule 5
		 'interface_names', 0, undef
	],
	[#Rule 6
		 'interface_names', 4,
sub
#line 36 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 7
		 'interface', 8,
sub
#line 40 "idl.yp"
{$_[3] => {
               "TYPE" => "INTERFACE", 
	       "PROPERTIES" => $_[1],
	       "NAME" => $_[3],
	       "BASE" => $_[4],
	       "DATA" => $_[6],
		   "FILE" => $_[0]->YYData->{INPUT_FILENAME},
		   "LINE" => $_[0]->YYData->{LINE},
          }}
	],
	[#Rule 8
		 'base_interface', 0, undef
	],
	[#Rule 9
		 'base_interface', 2,
sub
#line 53 "idl.yp"
{ $_[2] }
	],
	[#Rule 10
		 'definitions', 1,
sub
#line 57 "idl.yp"
{ [ $_[1] ] }
	],
	[#Rule 11
		 'definitions', 2,
sub
#line 58 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 12
		 'definition', 1, undef
	],
	[#Rule 13
		 'definition', 1, undef
	],
	[#Rule 14
		 'definition', 1, undef
	],
	[#Rule 15
		 'definition', 1, undef
	],
	[#Rule 16
		 'definition', 1, undef
	],
	[#Rule 17
		 'const', 7,
sub
#line 66 "idl.yp"
{{
                     "TYPE"  => "CONST", 
		     "DTYPE"  => $_[2],
			 "POINTERS" => $_[3],
		     "NAME"  => $_[4],
		     "VALUE" => $_[6],
		     "FILE" => $_[0]->YYData->{INPUT_FILENAME},
		     "LINE" => $_[0]->YYData->{LINE},
        }}
	],
	[#Rule 18
		 'const', 8,
sub
#line 76 "idl.yp"
{{
                     "TYPE"  => "CONST", 
		     "DTYPE"  => $_[2],
			 "POINTERS" => $_[3],
		     "NAME"  => $_[4],
		     "ARRAY_LEN" => $_[5],
		     "VALUE" => $_[7],
		     "FILE" => $_[0]->YYData->{INPUT_FILENAME},
		     "LINE" => $_[0]->YYData->{LINE},
        }}
	],
	[#Rule 19
		 'function', 7,
sub
#line 90 "idl.yp"
{{
		"TYPE" => "FUNCTION",
		"NAME" => $_[3],
		"RETURN_TYPE" => $_[2],
		"PROPERTIES" => $_[1],
		"ELEMENTS" => $_[5],
		"FILE" => $_[0]->YYData->{INPUT_FILENAME},
		"LINE" => $_[0]->YYData->{LINE},
	  }}
	],
	[#Rule 20
		 'declare', 5,
sub
#line 102 "idl.yp"
{{
	             "TYPE" => "DECLARE", 
                     "PROPERTIES" => $_[2],
		     "NAME" => $_[4],
		     "DATA" => $_[3],
		     "FILE" => $_[0]->YYData->{INPUT_FILENAME},
		     "LINE" => $_[0]->YYData->{LINE},
        }}
	],
	[#Rule 21
		 'decl_type', 1, undef
	],
	[#Rule 22
		 'decl_type', 1, undef
	],
	[#Rule 23
		 'decl_type', 1, undef
	],
	[#Rule 24
		 'decl_enum', 1,
sub
#line 116 "idl.yp"
{{
                     "TYPE" => "ENUM"
        }}
	],
	[#Rule 25
		 'decl_bitmap', 1,
sub
#line 122 "idl.yp"
{{
                     "TYPE" => "BITMAP"
        }}
	],
	[#Rule 26
		 'decl_union', 1,
sub
#line 128 "idl.yp"
{{
                     "TYPE" => "UNION"
        }}
	],
	[#Rule 27
		 'typedef', 6,
sub
#line 134 "idl.yp"
{{
	             "TYPE" => "TYPEDEF", 
                     "PROPERTIES" => $_[2],
		     "NAME" => $_[4],
		     "DATA" => $_[3],
		     "ARRAY_LEN" => $_[5],
		     "FILE" => $_[0]->YYData->{INPUT_FILENAME},
		     "LINE" => $_[0]->YYData->{LINE},
        }}
	],
	[#Rule 28
		 'usertype', 1, undef
	],
	[#Rule 29
		 'usertype', 1, undef
	],
	[#Rule 30
		 'usertype', 1, undef
	],
	[#Rule 31
		 'usertype', 1, undef
	],
	[#Rule 32
		 'typedecl', 2,
sub
#line 147 "idl.yp"
{ $_[1] }
	],
	[#Rule 33
		 'sign', 1, undef
	],
	[#Rule 34
		 'sign', 1, undef
	],
	[#Rule 35
		 'existingtype', 0, undef
	],
	[#Rule 36
		 'existingtype', 2,
sub
#line 152 "idl.yp"
{ "$_[1] $_[2]" }
	],
	[#Rule 37
		 'existingtype', 1, undef
	],
	[#Rule 38
		 'type', 1, undef
	],
	[#Rule 39
		 'type', 1, undef
	],
	[#Rule 40
		 'type', 1,
sub
#line 156 "idl.yp"
{ "void" }
	],
	[#Rule 41
		 'enum_body', 3,
sub
#line 158 "idl.yp"
{ $_[2] }
	],
	[#Rule 42
		 'opt_enum_body', 0, undef
	],
	[#Rule 43
		 'opt_enum_body', 1, undef
	],
	[#Rule 44
		 'enum', 3,
sub
#line 161 "idl.yp"
{{
             "TYPE" => "ENUM", 
			 "NAME" => $_[2],
		     "ELEMENTS" => $_[3]
        }}
	],
	[#Rule 45
		 'enum_elements', 1,
sub
#line 169 "idl.yp"
{ [ $_[1] ] }
	],
	[#Rule 46
		 'enum_elements', 3,
sub
#line 170 "idl.yp"
{ push(@{$_[1]}, $_[3]); $_[1] }
	],
	[#Rule 47
		 'enum_element', 1, undef
	],
	[#Rule 48
		 'enum_element', 3,
sub
#line 174 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 49
		 'bitmap_body', 3,
sub
#line 177 "idl.yp"
{ $_[2] }
	],
	[#Rule 50
		 'opt_bitmap_body', 0, undef
	],
	[#Rule 51
		 'opt_bitmap_body', 1, undef
	],
	[#Rule 52
		 'bitmap', 3,
sub
#line 180 "idl.yp"
{{
             "TYPE" => "BITMAP", 
			 "NAME" => $_[2],
		     "ELEMENTS" => $_[3]
        }}
	],
	[#Rule 53
		 'bitmap_elements', 1,
sub
#line 188 "idl.yp"
{ [ $_[1] ] }
	],
	[#Rule 54
		 'bitmap_elements', 3,
sub
#line 189 "idl.yp"
{ push(@{$_[1]}, $_[3]); $_[1] }
	],
	[#Rule 55
		 'opt_bitmap_elements', 0, undef
	],
	[#Rule 56
		 'opt_bitmap_elements', 1, undef
	],
	[#Rule 57
		 'bitmap_element', 3,
sub
#line 194 "idl.yp"
{ "$_[1] ( $_[3] )" }
	],
	[#Rule 58
		 'struct_body', 3,
sub
#line 197 "idl.yp"
{ $_[2] }
	],
	[#Rule 59
		 'opt_struct_body', 0, undef
	],
	[#Rule 60
		 'opt_struct_body', 1, undef
	],
	[#Rule 61
		 'struct', 3,
sub
#line 201 "idl.yp"
{{
             "TYPE" => "STRUCT", 
			 "NAME" => $_[2],
		     "ELEMENTS" => $_[3]
        }}
	],
	[#Rule 62
		 'empty_element', 2,
sub
#line 209 "idl.yp"
{{
		 "NAME" => "",
		 "TYPE" => "EMPTY",
		 "PROPERTIES" => $_[1],
		 "POINTERS" => 0,
		 "ARRAY_LEN" => [],
		 "FILE" => $_[0]->YYData->{INPUT_FILENAME},
		 "LINE" => $_[0]->YYData->{LINE},
	 }}
	],
	[#Rule 63
		 'base_or_empty', 2, undef
	],
	[#Rule 64
		 'base_or_empty', 1, undef
	],
	[#Rule 65
		 'optional_base_element', 2,
sub
#line 223 "idl.yp"
{ $_[2]->{PROPERTIES} = FlattenHash([$_[1],$_[2]->{PROPERTIES}]); $_[2] }
	],
	[#Rule 66
		 'union_elements', 0, undef
	],
	[#Rule 67
		 'union_elements', 2,
sub
#line 228 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 68
		 'union_body', 3,
sub
#line 231 "idl.yp"
{ $_[2] }
	],
	[#Rule 69
		 'opt_union_body', 0, undef
	],
	[#Rule 70
		 'opt_union_body', 1, undef
	],
	[#Rule 71
		 'union', 3,
sub
#line 235 "idl.yp"
{{
             "TYPE" => "UNION", 
		     "NAME" => $_[2],
		     "ELEMENTS" => $_[3]
        }}
	],
	[#Rule 72
		 'base_element', 5,
sub
#line 243 "idl.yp"
{{
			   "NAME" => $_[4],
			   "TYPE" => $_[2],
			   "PROPERTIES" => $_[1],
			   "POINTERS" => $_[3],
			   "ARRAY_LEN" => $_[5],
		       "FILE" => $_[0]->YYData->{INPUT_FILENAME},
		       "LINE" => $_[0]->YYData->{LINE},
              }}
	],
	[#Rule 73
		 'pointers', 0,
sub
#line 257 "idl.yp"
{ 0 }
	],
	[#Rule 74
		 'pointers', 2,
sub
#line 258 "idl.yp"
{ $_[1]+1 }
	],
	[#Rule 75
		 'element_list1', 0, undef
	],
	[#Rule 76
		 'element_list1', 3,
sub
#line 263 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 77
		 'element_list2', 0, undef
	],
	[#Rule 78
		 'element_list2', 1, undef
	],
	[#Rule 79
		 'element_list2', 1,
sub
#line 269 "idl.yp"
{ [ $_[1] ] }
	],
	[#Rule 80
		 'element_list2', 3,
sub
#line 270 "idl.yp"
{ push(@{$_[1]}, $_[3]); $_[1] }
	],
	[#Rule 81
		 'array_len', 0, undef
	],
	[#Rule 82
		 'array_len', 3,
sub
#line 275 "idl.yp"
{ push(@{$_[3]}, "*"); $_[3] }
	],
	[#Rule 83
		 'array_len', 4,
sub
#line 276 "idl.yp"
{ push(@{$_[4]}, "$_[2]"); $_[4] }
	],
	[#Rule 84
		 'property_list', 0, undef
	],
	[#Rule 85
		 'property_list', 4,
sub
#line 282 "idl.yp"
{ FlattenHash([$_[1],$_[3]]); }
	],
	[#Rule 86
		 'properties', 1,
sub
#line 285 "idl.yp"
{ $_[1] }
	],
	[#Rule 87
		 'properties', 3,
sub
#line 286 "idl.yp"
{ FlattenHash([$_[1], $_[3]]); }
	],
	[#Rule 88
		 'property', 1,
sub
#line 289 "idl.yp"
{{ "$_[1]" => "1"     }}
	],
	[#Rule 89
		 'property', 4,
sub
#line 290 "idl.yp"
{{ "$_[1]" => "$_[3]" }}
	],
	[#Rule 90
		 'listtext', 1, undef
	],
	[#Rule 91
		 'listtext', 3,
sub
#line 295 "idl.yp"
{ "$_[1] $_[3]" }
	],
	[#Rule 92
		 'commalisttext', 1, undef
	],
	[#Rule 93
		 'commalisttext', 3,
sub
#line 300 "idl.yp"
{ "$_[1],$_[3]" }
	],
	[#Rule 94
		 'anytext', 0,
sub
#line 304 "idl.yp"
{ "" }
	],
	[#Rule 95
		 'anytext', 1, undef
	],
	[#Rule 96
		 'anytext', 1, undef
	],
	[#Rule 97
		 'anytext', 1, undef
	],
	[#Rule 98
		 'anytext', 3,
sub
#line 306 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 99
		 'anytext', 3,
sub
#line 307 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 100
		 'anytext', 3,
sub
#line 308 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 101
		 'anytext', 3,
sub
#line 309 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 102
		 'anytext', 3,
sub
#line 310 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 103
		 'anytext', 3,
sub
#line 311 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 104
		 'anytext', 3,
sub
#line 312 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 105
		 'anytext', 3,
sub
#line 313 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 106
		 'anytext', 3,
sub
#line 314 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 107
		 'anytext', 3,
sub
#line 315 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 108
		 'anytext', 3,
sub
#line 316 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 109
		 'anytext', 3,
sub
#line 317 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 110
		 'anytext', 3,
sub
#line 318 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 111
		 'anytext', 5,
sub
#line 319 "idl.yp"
{ "$_[1]$_[2]$_[3]$_[4]$_[5]" }
	],
	[#Rule 112
		 'anytext', 5,
sub
#line 320 "idl.yp"
{ "$_[1]$_[2]$_[3]$_[4]$_[5]" }
	],
	[#Rule 113
		 'identifier', 1, undef
	],
	[#Rule 114
		 'optional_identifier', 1, undef
	],
	[#Rule 115
		 'optional_identifier', 0, undef
	],
	[#Rule 116
		 'constant', 1, undef
	],
	[#Rule 117
		 'text', 1,
sub
#line 334 "idl.yp"
{ "\"$_[1]\"" }
	],
	[#Rule 118
		 'optional_semicolon', 0, undef
	],
	[#Rule 119
		 'optional_semicolon', 1, undef
	]
],
                                  @_);
    bless($self,$class);
}

#line 345 "idl.yp"


#####################################################################
# flatten an array of hashes into a single hash
sub FlattenHash($) 
{ 
    my $a = shift;
    my %b;
    for my $d (@{$a}) {
	for my $k (keys %{$d}) {
	    $b{$k} = $d->{$k};
	}
    }
    return \%b;
}



#####################################################################
# traverse a perl data structure removing any empty arrays or
# hashes and any hash elements that map to undef
sub CleanData($)
{
    sub CleanData($);
    my($v) = shift;
	return undef if (not defined($v));
    if (ref($v) eq "ARRAY") {
	foreach my $i (0 .. $#{$v}) {
	    CleanData($v->[$i]);
	    if (ref($v->[$i]) eq "ARRAY" && $#{$v->[$i]}==-1) { 
		    $v->[$i] = undef; 
		    next; 
	    }
	}
	# this removes any undefined elements from the array
	@{$v} = grep { defined $_ } @{$v};
    } elsif (ref($v) eq "HASH") {
	foreach my $x (keys %{$v}) {
	    CleanData($v->{$x});
	    if (!defined $v->{$x}) { delete($v->{$x}); next; }
	    if (ref($v->{$x}) eq "ARRAY" && $#{$v->{$x}}==-1) { delete($v->{$x}); next; }
	}
    }
	return $v;
}

sub _Error {
    if (exists $_[0]->YYData->{ERRMSG}) {
		print $_[0]->YYData->{ERRMSG};
		delete $_[0]->YYData->{ERRMSG};
		return;
	};
	my $line = $_[0]->YYData->{LINE};
	my $last_token = $_[0]->YYData->{LAST_TOKEN};
	my $file = $_[0]->YYData->{INPUT_FILENAME};
	
	print "$file:$line: Syntax error near '$last_token'\n";
}

sub _Lexer($)
{
	my($parser)=shift;

    $parser->YYData->{INPUT} or return('',undef);

again:
	$parser->YYData->{INPUT} =~ s/^[ \t]*//;

	for ($parser->YYData->{INPUT}) {
		if (/^\#/) {
			if (s/^\# (\d+) \"(.*?)\"( \d+|)//) {
				$parser->YYData->{LINE} = $1-1;
				$parser->YYData->{INPUT_FILENAME} = $2;
				goto again;
			}
			if (s/^\#line (\d+) \"(.*?)\"( \d+|)//) {
				$parser->YYData->{LINE} = $1-1;
				$parser->YYData->{INPUT_FILENAME} = $2;
				goto again;
			}
			if (s/^(\#.*)$//m) {
				goto again;
			}
		}
		if (s/^(\n)//) {
			$parser->YYData->{LINE}++;
			goto again;
		}
		if (s/^\"(.*?)\"//) {
			$parser->YYData->{LAST_TOKEN} = $1;
			return('TEXT',$1); 
		}
		if (s/^(\d+)(\W|$)/$2/) {
			$parser->YYData->{LAST_TOKEN} = $1;
			return('CONSTANT',$1); 
		}
		if (s/^([\w_]+)//) {
			$parser->YYData->{LAST_TOKEN} = $1;
			if ($1 =~ 
			    /^(coclass|interface|const|typedef|declare|union
			      |struct|enum|bitmap|void|unsigned|signed)$/x) {
				return $1;
			}
			return('IDENTIFIER',$1);
		}
		if (s/^(.)//s) {
			$parser->YYData->{LAST_TOKEN} = $1;
			return($1,$1);
		}
	}
}

sub parse_string
{
	my ($data,$filename) = @_;

	my $self = new Parse::Pidl::IDL;

    $self->YYData->{INPUT_FILENAME} = $filename;
    $self->YYData->{INPUT} = $data;
    $self->YYData->{LINE} = 0;
    $self->YYData->{LAST_TOKEN} = "NONE";

	my $idl = $self->YYParse( yylex => \&_Lexer, yyerror => \&_Error );

	return CleanData($idl);
}

sub parse_file($$)
{
	my ($filename,$incdirs) = @_;

	my $saved_delim = $/;
	undef $/;
	my $cpp = $ENV{CPP};
	if (! defined $cpp) {
		$cpp = "cpp";
	}
	my $includes = map { " -I$_" } @$incdirs;
	my $data = `$cpp -D__PIDL__$includes -xc $filename`;
	$/ = $saved_delim;

	return parse_string($data, $filename);
}

1;
