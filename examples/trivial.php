Begin <br>
<script language="php">
dl ("gprolog.so");

// pl_debug(1);
pl_debug(1);

echo "<H2>Open</H2> <br>\n";
$pl = pl_open ("/usr/bin/gprolog-php", "gprolog-php");
echo "<H2>Opened: pl=$pl</H2> <br>\n";

echo "<H2>Close</H2> <br>\n";
pl_close($pl);
echo "<H2>Closed</H2> <br>\n";

</script>
End <br>
