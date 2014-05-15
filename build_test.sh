#!/bin/sh

for ruby in \
 1.8.7-p374\
 rbx-2.2.6\
 ree-1.8.7-2012.02\
 1.9.3-p484\
 2.0.0-p353\
 2.1.1\
 2.1.2
do
    echo "\n********************************************************************************"
    echo "Building $ruby\n"
    cd ext/oj
    rbenv local $ruby
    ruby extconf.rb
    make

    echo "\nRunning tests for $ruby"
    cd ../../test
    rbenv local $ruby
    ./test_all.sh
    cd ..

    echo "\n"
done

PATH=/System/Library/Frameworks/Ruby.framework/Versions/1.8/usr/bin:$PATH
echo "\n********************************************************************************"
echo "Building OS X Ruby\n"
cd ext/oj
ruby extconf.rb
make

echo "\nRunning tests for OS X Ruby"
cd ../../test
./test_all.sh
cd ..

echo "\n"
