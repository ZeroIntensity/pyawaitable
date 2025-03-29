import optparse
import sys
import asyncio
import traceback


def main():
    parser = optparse.OptionParser()
    opts, args = parser.parse_args()

    if not args:
        parser.error("Expected one argument.")

    mod = args[0]

    called = False
    async def dummy():
        await asyncio.sleep(0)
        nonlocal called
        called = True

    try:
        asyncio.run(__import__(mod).async_function(dummy()))
    except BaseException as err:
        traceback.print_exc()
        print("Build failed!", file=sys.stderr)
        sys.exit(1)
    
    if not called:
        print("Build doesn't work!", file=sys.stderr)
        sys.exit(1)
    
    print("Success!")

if __name__ == "__main__":
    main()