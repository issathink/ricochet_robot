#!/usr/bin/perl -l
open(F,"./src/log.ini");
	my %hash= ();
	while($ligne=<F>){
		if($ligne =~/((\w|\d)+) (\d+)/){
			print STDOUT "j'ai trouve $1 et $3";
			$hash{$1}= $hash{$1} + $3;
		}
	}
close F;
open(F2,">./src/log.ini");
foreach my $e(sort { $hash{$b} <=> $hash{$a}}keys %hash){
	print F2 "$e $hash{$e}";
}
close F2;
open (F3,">score.html");
   $html="<HTML><HEAD><TITLE> Score board for our application </TITLE></HEAD><BODY>";
   $values="<h1> Tableau des scores </h1>";
   foreach my $e(sort { $hash{$b} <=> $hash{$a}}keys %hash){
	$values.="<h3> $e $hash{$e} points</h3>";
       }
    $html.=$values;
    $html.="</BODY></HTML>";
    print F3 $html;
close F3;

print STDOUT "-------------------------------------------";

