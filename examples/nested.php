Begin <br>
<script language="php">
dl ("gprolog.so");

pl_debug(1);
pl_debug(0);

echo "Open <br>\n";
$pl = pl_open ("/usr/bin/gprolog-php-cx", "gprolog-php-cx");

echo "Query <br>\n";
pl_query ($pl, "member(X, [1,2,3])");
while (pl_more ($pl)) {
  pl_query($pl, "member(Y, [a,b])");
  while (pl_more ($pl)) {
    echo "Solution -> X = $X, Y = $Y. <br>\n";
  }
}

pl_close($pl);

</script>
End <br>
