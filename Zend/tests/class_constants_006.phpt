--TEST--
Ownership of constant expression inherited from interface should be tranferred to class
--FILE--
<?php
interface I {
    const X2 = 'X' . self::Y2;
    const Y2 = 'Y';
}
eval('class B implements I{}');
var_dump(B::X2);
?>
--EXPECT--
string(2) "XY"
