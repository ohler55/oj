#!/bin/sh

for ruby in \
 1.8.7-p352\
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
    make clean
    make

    echo "\nRunning tests for $ruby"
    cd ../../test
    rbenv local $ruby
    ./tests.rb
    cd ..

    echo "\n"
done
