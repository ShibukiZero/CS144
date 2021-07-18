Lab 6 Writeup
=============

My name: Lin Zihan

My SUNet ID: 18308125

This lab took me about 4 hours to do. I did not attend the lab session.

Program Structure and Design of the Router:
I use a struct RoutingRule to store routing rules, and use a vector to manage it.
I calculate the matching length by XORing the target ip and the ip of the corresponding 
forwarding entry, and then realize the longest prefix matching algorithm. After the 
destination ip and the ip address of the forwarding entry are XORed, the corresponding 
bit result is 0. At this time, if the longest prefix matches, the result of the XOR must 
be 1 after the first prefix_length bits are 0. Come the little ones. This achieves the 
longest prefix match.

Implementation Challenges:
Longest prefix match algorithm

Remaining Bugs:
None

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
