/./ { for ( i = 1 ; i <= NF ; ++i ) addword($i); }
/^$/ { printline(); print""; } 
END  { printline(); } 

function addword(word)
{
	if ( length(line) + 1 + length(word) > 60 )
		printline();
	if ( length(line) == 0 )
		line = word;
	else
		line = line " " word;
}


function printline()
{
	if ( length(line) > 0 ) {
		print line;
		line = "";
	}
}
