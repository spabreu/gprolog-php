Begin <br>
<script language="php">
dl ("gprolog.so");

pl_debug(0);			// 1=general 2=fb_ 4=select 8=parser

//echo "<H2>Open</H2> <br>\n";
$pl = pl_open ("/usr/bin/gprolog-php-cx", "gprolog-php-cx");

echo "<H2>Simple Query 1</H2> <br>\n";
pl_query_single ($pl, "write('Hello world!<br>'), nl");
echo "<H2>Done</H2> <br>\n";

echo "<H2>Simple Query 2</H2> <br>\n";
pl_query_single ($pl, "member(X, [a,b])");
echo "<b>single member(X, [a,b]) -> X = $X.</b><br>\n";
echo "<H2>Done</H2> <br>\n";

echo "<H2>ND Query 1</H2> <br>\n";
echo "<b>multiple member(X, [1,2,3]):</b><br>\n";
pl_query ($pl, "member(X, [1,2,3])");
while (pl_more ($pl)) {
  echo "<b>Solution -> X = $X.</b><br>\n";
}
echo "<H2>Done</H2> <br>\n";

echo "<H2>Table of known operators</H2> <br>\n";
echo "<table border=1>\n";
echo " <tr> <th>Operator</th> <th>Associativity</th> <th>Precedence</th> </tr>\n";
pl_query ($pl, "current_op(PREC, TYPE, OP)");
while (pl_more ($pl)) {
  echo "<tr> <td>" . htmlspecialchars($OP) . "</td> <td>$TYPE</td> <td>$PREC</td> </tr>\n";
}
echo "</table>\n";

pl_close($pl);

</script>
End <br>
