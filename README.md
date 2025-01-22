To test, run:

    make
    ./multicat -s <DELAY_IN_MICROSECONDS>

You will be able to type, with a set delay before keystrokes appear. The minimum testable delay is around 100, because the program itself has some overhead.

(There are 1000 microsecond in one millisecond, and 1000 milliseconds in one second)
