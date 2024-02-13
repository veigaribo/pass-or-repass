# Pass or Repass

Professor:

> You guys have to implement a game like the game show [Passa ou Repassa](https://pt.wikipedia.org/wiki/Passa_ou_Repassa) using the socket API in basically any language where it exists.

Me:

> Hm what if I write it in C with some kind of event loop that consumes the events with multiple threads from a thread pool?

🙂

The code originally was very bad, I have since rewritten it.

It however most certainly still contains many race conditions and probably a few leaks, in addition to the questionable design choices, but my mental health comes first.

To compile the server, run the default `make` rule, optionally with the `DEBUG` option:

``` shell
$ make DEBUG=1
```

Or use the provided `Containerfile` using your favourite container image build tool.

The server is built to run on Linux exclusively, since it relies on `epoll`. But besides that, `libc` and `pthreads` support, which should all be available from a clean OS install, there are no dependencies.

The resulting `por` executable will start the server on port 10000, IPv4 address 0.0.0.0, using TCP. If it was not built with `DEBUG`, it will be quiet -- that's normal.

To connect and interact with it, use the client written in Python:

``` shell
$ python3 client/main.py
```

By default it will connect to `localhost` on port 10000, but you can change those via flags. Try `$ python3 client/main.py --help`.

No dependencies there either, besides a recent version of Python (>=3.10 I think) and its standard library.
