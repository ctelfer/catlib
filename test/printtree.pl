#!/usr/bin/perl

$max = 0;

while (<>) { 
	if ( /^node (\d+) = (0x[0-9a-fA-F]+)/ ) {
		$Nodes{$2} = $1;
	} elsif ( /^edge (\d+) = (0x[0-9a-fA-F]+): from (0x[0-9a-fA-F]+) on (\w|@)/ ) {
		$EdgePtr{$1} = $2;
		$EdgePar{$2} = $3;
		$EdgeChar{$2} = $4;
		if ( $1 > $max ) {
			$max = $1;
		}
	} elsif ( /^edge (0x[0-9a-fA-F]+) -> (0x[0-9a-fA-F]+)/ ) {
		$EdgeDst{$1} = $2;
	} else {
		print "Warning:  Didn't process |$_|\n";
	}
}

print "Edge\tFrom\tTo\tOn\n";
for ( $i = 0 ; $i <= $max ; $i++ ) {
	$key = $EdgePtr{$i};
	$par = $Nodes{$EdgePar{$key}};
	$dst = $Nodes{$EdgeDst{$key}};
	$char = $EdgeChar{$key};
	if ( $char eq '\0' ) {
		$char = "null";
	}
	print "$i\t$par\t$dst\t$char\n";
}
