Lab 3 Writeup
=============

My name: Lin Zihan

My SUNet ID: 18308125

This lab took me about 12 hours to do. I did not attend the lab session.

I worked with or talked about this assignment with: None

Program Structure and Design of the TCPSender:

TCP sender has three public interface, fill_window, ack_received
and tick. fill_window just transmit data as much as possible
no matter the real status of receiver. ack_received method
update window size and delete any segment fully acknowledged.
tick checks whether a segment is timed out, if so, retransmit segment
and execute backoff strategy.

Implementation Challenges:
Many design requirement is not mentioned in pdf,
I need to find out in test cases.
setup of SYN and FIN flags are really confusing.

Remaining Bugs:
All test passed

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
