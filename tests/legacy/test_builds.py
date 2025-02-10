import pytest
import subprocess
import sys
from pathlib import Path
import asyncio

BUILDS = Path(__file__).parent / "builds"


@pytest.mark.parametrize("build_system", ["setuptools"])
@pytest.mark.asyncio
async def test_builds(build_system: str):
    try:
        import _pyawaitable_test
    except ImportError:
        pass
    else:
        # Uninstall previous version
        subprocess.call(
            [
                sys.executable,
                "-m",
                "pip",
                "uninstall",
                "_pyawaitable_test",
                "--yes",
            ]
        )

    subprocess.call(
        [
            sys.executable,
            "-m",
            "pip",
            "install",
            str(BUILDS / build_system),
            "--no-build-isolation",
        ]
    )
    import _pyawaitable_test

    async def coro():
        await asyncio.sleep(0)
        return 42

    assert await _pyawaitable_test.test(coro()) == 42
