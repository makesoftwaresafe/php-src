--TEST--
Phar object: iterating via SplFileObject
--EXTENSIONS--
phar
--INI--
phar.require_hash=0
--FILE--
<?php

$pharconfig = 1;

require_once 'files/phar_oo_test.inc';

$phar = new Phar($fname);
$phar->setInfoClass('SplFileObject');

$f = $phar['a.csv'];
echo "===1===\n";
foreach($f as $k => $v)
{
    echo "$k=>$v\n";
}

$f->setFlags(SplFileObject::DROP_NEW_LINE);

echo "===2===\n";
foreach($f as $k => $v)
{
    echo "$k=>$v\n";
}

class MyCSVFile extends SplFileObject
{
    function current(): array|false
    {
        return parent::fgetcsv(',', '"', escape: '');
    }
}

$phar->setInfoClass('MyCSVFile');
/** @var MyCSVFile $v */
$v = $phar['a.csv'];

echo "===3===\n";
while(!$v->eof())
{
    echo $v->key() . "=>" . join('|', $v->fgetcsv(escape: '')) . "\n";
}

echo "===4===\n";
$v->rewind();
while(!$v->eof())
{
    $l = $v->fgetcsv(escape: '');
    echo $v->key() . "=>" . join('|', $l) . "\n";
}

echo "===5===\n";
foreach($v as $k => $d)
{
    echo "$k=>" . join('|', $d) . "\n";
}

class MyCSVFile2 extends SplFileObject
{
    function getCurrentLine(): string
    {
        echo __METHOD__ . "\n";
        return implode('|', parent::fgetcsv(',', '"', escape: ''));
    }
}

$phar->setInfoClass('MyCSVFile2');
/** @var MyCSVFile2 $v */
$v = $phar['a.csv'];

echo "===6===\n";
foreach($v as $k => $d)
{
    echo "$k=>" . $d . "\n";
}

?>
--CLEAN--
<?php
unlink(__DIR__ . '/files/phar_oo_008.phar.php');
__halt_compiler();
?>
--EXPECT--
===1===
0=>1,2,3

1=>2,a,b

2=>3,"c","'e'"
===2===
0=>1,2,3
1=>2,a,b
2=>3,"c","'e'"
===3===
0=>1|2|3
0=>2|a|b
1=>3|c|'e'
===4===
0=>1|2|3
1=>2|a|b
2=>3|c|'e'
===5===
0=>1|2|3
1=>2|a|b
2=>3|c|'e'
===6===
MyCSVFile2::getCurrentLine
1=>1|2|3
MyCSVFile2::getCurrentLine
3=>2|a|b
MyCSVFile2::getCurrentLine
5=>3|c|'e'
