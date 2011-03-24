#!/usr/bin/php
<?

$plot = "";
$plot .= "set logscale xy 10\n";
$plot .= "set datafile separator \",\"\n";

$parts = array();
array_shift($argv);
foreach($argv as $dir)
	$parts[] = "\"$dir/timestats.csv\" using 2:3 with lines title \"$dir\"";

$plot .= "plot " . implode(', ', $parts);


$tmpfname = tempnam("/tmp", "plot");

$fd = fopen($tmpfname, "w");
fwrite($fd, $plot);
fclose($fd);

`gnuplot -raise -persist $tmpfname`;
unlink($tmpfname);



