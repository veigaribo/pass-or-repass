do_output = False


def debug(prefix, *debug_args, **debug_kwargs):
    if do_output:
        print(f'[{prefix}]', *debug_args, **debug_kwargs)


def set_debug_output(value: bool):
    global do_output
    do_output = value
