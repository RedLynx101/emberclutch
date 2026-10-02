"""ElevenLabs for the trailer's narration (docs/plan/trailer.md): the account's credits, its voices, and speech.

    py -3.12 tools/film/eleven.py credits
    py -3.12 tools/film/eleven.py voices [filter]
    py -3.12 tools/film/eleven.py say <voice_id> <out.mp3> <text> [--model eleven_v4]

The key is the Windows user environment variable ELEVENLABS_API_KEY (read from the registry: a process's own
environment can be stale), sent as the xi-api-key header and never printed.
"""
import json
import sys
import urllib.error
import urllib.request
import winreg

API = "https://api.elevenlabs.io"


def key() -> str:
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, "Environment") as k:
        return winreg.QueryValueEx(k, "ELEVENLABS_API_KEY")[0]


def call(path: str, body=None, accept="application/json"):
    req = urllib.request.Request(API + path, data=json.dumps(body).encode() if body is not None else None,
                                 method="POST" if body is not None else "GET",
                                 headers={"xi-api-key": key(), "Content-Type": "application/json", "Accept": accept})
    try:
        with urllib.request.urlopen(req, timeout=120) as r:
            return r.read()
    except urllib.error.HTTPError as e:
        sys.exit(f"[eleven] HTTP {e.code}: {e.read()[:400].decode(errors='replace')}")


def main() -> None:
    what = sys.argv[1] if len(sys.argv) > 1 else "credits"
    if what == "credits":
        s = json.loads(call("/v1/user/subscription"))
        print(f"[eleven] {s.get('tier')}: {s['character_count']:,} of {s['character_limit']:,} credits used, "
              f"resets {s.get('next_character_count_reset_unix')}")
    elif what == "voices":
        flt = sys.argv[2].lower() if len(sys.argv) > 2 else ""
        token, n = None, 0
        while True:
            q = "/v2/voices?page_size=100" + (f"&next_page_token={token}" if token else "")
            v = json.loads(call(q))
            for voice in v.get("voices", []):
                labels = voice.get("labels") or {}
                line = f"{voice['voice_id']}  {voice['name']:<28} {voice.get('category', ''):<10} " + \
                       ", ".join(f"{k}={val}" for k, val in labels.items())
                if flt in line.lower():
                    print(line)
                    n += 1
            token = v.get("next_page_token")
            if not v.get("has_more") or not token:
                break
        print(f"[eleven] {n} voices")
    elif what == "models":
        for m in json.loads(call("/v1/models")):
            print(m["model_id"], "-", m.get("name"))
    elif what == "say":
        voice, out, text = sys.argv[2], sys.argv[3], sys.argv[4]
        model = sys.argv[sys.argv.index("--model") + 1] if "--model" in sys.argv else "eleven_v4"
        audio = call(f"/v1/text-to-speech/{voice}?output_format=mp3_44100_128", {"text": text, "model_id": model},
                     accept="audio/mpeg")
        open(out, "wb").write(audio)
        print(f"[eleven] {out}: {len(audio):,} bytes ({len(text)} characters)")


if __name__ == "__main__":
    main()
