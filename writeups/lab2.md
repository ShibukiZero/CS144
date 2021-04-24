Lab 2 Writeup
=============

My name: Lin Zihan

My SUNet ID: 18308125

This lab took me about 9 hours to do. I did not attend the lab session.

I worked with or talked about this assignment with: None

Program Structure and Design of the TCPReceiver and wrap/unwrap routines:

TCP receiver is designed to receive TCP segments and ack them. When a segment arrives
in TCP, it first set connection if there is a syn flag. If connected, TCP
receiver unwrap sequence number and omit syn sequence number. If it received a 
fin flag, it set eof to true. There is a private attribution to record the left 
edge of TCP receiver. When acking, it returns this attribution, including fin flag.

Wrap is designed simply by calculating the remainder of absolute sequence number
divided by 2^32, and add isn.

Unwrap is designed by finding the right position of wrapped sequence number in one
round trip, and later by finding the right round trip, we can get the absolute 
sequence number closest to checkpoint.

Implementation Challenges:

When first implementing unwrap, it's very slow to run. It turns out I wrote
the while loop that is too computationally expensive. The code submitted applied
a much faster approach.

Remaining Bugs:
All test passed

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
