#!/bin/sh

for ruby in \
 1.8.7-p358\
 1.9.2-p290\
 1.9.3-p125\
 jruby-1.6.7\
 rbx-1.2.4\
 rbx-2.0.0-dev\
 ree-1.8.7-2012.02
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
    ./tests.rb
    ./test_mimic.rb
    ./test_fast.rb
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
./tests.rb
./test_mimic.rb
./test_fast.rb
cd ..

echo "\n"
