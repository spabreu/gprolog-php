Begin <br>
<script language="php">
dl ("gprolog.so");

pl_debug(1);
pl_debug(0);

echo "Open <br>\n";
$pl = pl_open ("/home/spa/work/src/isco/php-pix", "php-pix");

echo "Query <br>\n";
pl_query_single ($pl, "write('ola <br>'), nl");
pl_query_all ($pl, "member(X, [1,2,3]), write(ola), nl");
while (pl_more ($pl)) {
  echo "Solution -> X = $X. <br>\n";
}

echo "Auto <br>\n";
pl_query_all ($pl, "member(X, [1,2,3]), output_html([heading(X, 'Hello')]), nl");
while (pl_more ($pl)) {
  echo "Solution -> X = $X. <br>\n";
}

echo "Table <br>\n";
echo "<table border=1>\n";
echo " <tr> <th>Field</th> <th>Type</th> <th>Position</th> </tr>\n";
$query =
  "isco_field(coolpix_pix, FIELD, POS, TYPE), " .
  "output_html(tr([td(FIELD), td(TYPE), td(POS)])), nl";
pl_query_all ($pl, $query);
while (pl_more ($pl)) {}
echo "</table>\n";

echo "All of them<br>\n";
echo "<table border=1>\n";
echo " <tr> <th>File</th> <th>ISO</th> <th>Shutter</th> <th>Aperture</th> </tr>\n";
$query =
  "coolpix_pix(file_name=F asc, sensitivity=S, shutter=SP, aperture=AP), " .
  "output_html(tr([td(F), td(S), td(SP), td(AP)])), nl";
pl_query_all ($pl, $query);
while (pl_more ($pl)) {}
echo "</table>\n";


pl_close($pl);

</script>
End <br>
