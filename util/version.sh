#
let arg=$#
let arg=$(($arg - 2))
ver=$1
shift
for ((i = 0; i < $arg; i += 1));
do
ver=$ver.$1
shift
done
val=$1
val=$(($val+1))
ver=$ver.$val
echo $ver
