#!/usr/bin/perl

@alphabet = ( 'a' .. 'c' );

sub gen_rand_string { 
	my $len = shift;
	my $str = join('', map($alphabet[rand(@alphabet)], 0..$len));
}

sub run_test {
	$len = int(rand(10)) + 10;
	$str = gen_rand_string($len);
	my $sublen = rand($len) + 1;
	my $off = rand($sublen);
	my $substr = substr($str, $off, $sublen);

	open(RUN, "./testmatch -s $str $substr 2>&1 |");
	my $res = "";
	while ( <RUN> ) { $res .= $_; } 
	close(RUN);

	if ( $res =~ /(Assertion|not found)/ ) {
		print "Found Error!\n";
		print "string = $str, pattern = $substr\n";
		print "result = $res";
	}
}

for ( $i = 0 ; $i < 1000 ; $i++ ) { 
	run_test(); 
}
