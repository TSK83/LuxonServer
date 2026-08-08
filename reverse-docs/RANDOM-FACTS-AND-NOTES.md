# Random facts about Photon and Exit Games

First of all: **Photon, Photon Realtime and Exit Games are a trademark or registered trademark of Exit Games. All other product and company names mentioned herein are used for identification purposes only and may be trademarks or registered trademarks of their respective owners.**


## ENet

Photon runs on a custom ENet implementation. It might be the most beautiful part of the protocol, it's really really well made and has a ton of improvement over the "original" open source ENet protocol implemented by [`libenet`](https://github.com/libnet/libnet).


## [ServerFramework](https://serverframework.com) and [Len Holgate](https://www.linkedin.com/in/len-holgate-515495)

According to my research, the custom ENet protocol was mostly or at least in part designed by *Len Holgate* who appears to be a contractor according to [his blog](https://lenholgate.com). If you'd like to learn more, start here:

### [.Net 4.0 Hosting](https://lenholgate.com/blog/2009/10/net-40-hosting.html)
> My super secret game company client need their ENet implementation to run fast and to support 1000s of concurrent connections.

### [Debugging yourself...](https://lenholgate.com/blog/2024/04/debugging-yourself.html)
> A long time ago I wrote a server system for a games company. The server is written in C++ and runs on Windows and hosts the Microsoft CLR, which, in turn, runs managed code for the actual game code. The native C++ does the networking with various protocols and, even after all these years, is still, predominantly, my domain. The C# code is written by the game company’s developers. We now also run on Linux using .Net Core, but the distribution of responsibility is still the same; I do the native stuff; they do the managed stuff.

## Photon [Serialization](https://doc.photonengine.com/realtime/current/reference/serialization-in-photon)

This was definitely the most difficult part for me to understand. So as it turns out, there's more than one serialization! There is the the "GP Binary Serialization" at the link above... which is actually 2 serializations! 

There's version *1.6* (commonly referred to as just "16") and version *1.8* (commonly referred to as just "18").

Version *1.6* hasn't been in use for quite a while (cutoff seems to be around 2018?) and is less efficient. Version *1.8* uses tricks to encode common *values* (like true, false, 0, 0.0, etc.) into the *type* field amongst other improvements.

Then there also appears to be JSON serialization for web based stuff according to some folks I spoke to, but I couldn't find a game that uses it so I haven't been able to implement it.

*Many thanks to the dude who helped me figure the basics of version 1.8 and ENet out! All this wouldn't have been possible without you! You know who you are if you're reading this :-)*

### "But is it good?"

Well... 1.6 is a bit of a joke. They've made some really odd decisions there. 1.8 however is just fine. Nothing too exciting.

## Photon "Realtime"

On top of ENet and that serialization protocol sits "Photon Realtime". What makes it "realtime"? Probably the protocols it sits on top of. Other than that it's just a marketing name for a load balancing and event delivery protocol.

It generally makes a lot of sense. The choice of data types seems to be a bit random in places.

I should however note that they've really missed out on a very simple yet effective optimization here: Event data may require decoding and can't be treated by the server as a single opaque blob most of the time. This means that even if the server doesn't need to inspect/manipulate its contents it *at all*, it often needs to spend time decoding and re-encoding event data for each recipient. At least the encoding part is cached by Luxon Server to speed this up if there are a several clients.

It looks like newer Photon Realtime based libraries like Fusion and Quantum have gotten this part right though.

### Optional limitations

Exit Games allows developers to put certain restrictions on the Photon Realtime protocol in place on email request.

So far, I have found the following options from searching public forums etc.:

 - Blocked FindFriends opcode (to prevent stalking)
 - Blocked MasterClientId setter (so master client can only change when current master client leaves)

## Photon Quantum

From how I understand the protocol by just by dissecting its event payloads:

There's a room-wide "network frame" counter, and each client is expected to send its inputs within a specific time frame. Finally, all clients receive all inputs of all other clients, and the next frame begins. There are mechanisms in place to handle lagging clients (if they miss frames they receive input data for all these missed frames all at once), and there's a lot of zigzag encoding and gzip compression going on.

I have not yet been able to implement a working Quantum plugin for Luxon Server yet because of how complex the encoding is.

## Photon Fusion

Fusion is similarly complex, while additionally implementing a custom reliability protocol (replacing ENet reliability). I have documented it extensively here: [Fusion Binary Protocol](/reverse-docs/fusion-traffic-observations.md) (you can also look at some captured traffic here: [Fusion Traffic](/reverse-docs/fusion-traffic.txt)).
