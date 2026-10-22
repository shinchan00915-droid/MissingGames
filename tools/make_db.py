#!/usr/bin/env python3
"""hax0kartik/3dsdb 의 국가별 JSON을 받아 db.txt 생성.
출력(탭 구분): titleid, region(K/U/E/J/T), group, english name, original name
group = 제품코드(CTR-P-AMKE)의 게임코드 앞 3글자 -> 지역이 달라도 같은 게임이면 같은 값
"""
import json, re, sys, unicodedata, urllib.request

BASE = "https://raw.githubusercontent.com/hax0kartik/3dsdb/master/jsons/list_%s.json"
REGIONS = [("US", "U"), ("GB", "E"), ("KR", "K"), ("JP", "J"), ("TW", "T")]
PRIORITY = {"U": 0, "E": 1, "K": 2, "J": 3, "T": 4}


def ascii_name(s):
    s = s.replace("\u2122", "").replace("\u00ae", "")
    s = unicodedata.normalize("NFKD", s).encode("ascii", "ignore").decode()
    s = re.sub(r"\(\s*\)", "", s)
    s = re.sub(r"\s+", " ", s).strip(" -")
    return s


def clean(s):
    return re.sub(r"[\t\r\n]+", " ", s).strip()


def main(out="db.txt"):
    seen, rows = set(), []
    for lst, reg in REGIONS:
        with urllib.request.urlopen(BASE % lst, timeout=60) as r:
            data = json.loads(r.read().decode("utf-8"))
        for d in data:
            tid = d.get("TitleID", "").upper()
            if not re.fullmatch(r"00040000[0-9A-F]{8}", tid) or tid in seen:
                continue  # 게임 본편(00040000)만. 업데이트/DLC/체험판 제외
            seen.add(tid)
            code = d.get("Product Code", "")
            m = re.fullmatch(r"CTR-[A-Z0-9]-([A-Z0-9]{4})", code)
            grp = m.group(1)[:3] if m else tid[8:14]
            orig = clean(d.get("Name", ""))
            en = ascii_name(orig)
            if len(re.findall(r"[A-Za-z0-9]", en)) < 3:
                en = ""
            rows.append(dict(tid=tid, reg=reg, grp=grp, en=en, orig=orig, code=code))

    best = {}
    for r in sorted(rows, key=lambda x: PRIORITY[x["reg"]]):
        if r["en"] and r["grp"] not in best:
            best[r["grp"]] = r["en"]
    for r in rows:
        r["name"] = best.get(r["grp"]) or r["en"] or r["code"] or r["tid"]
        r["name"] = r["name"][:70]
        r["orig"] = r["orig"][:90]

    rows.sort(key=lambda x: (x["name"].lower(), PRIORITY[x["reg"]]))
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write("# titleid\tregion\tgroup\tenglish name\toriginal name\n")
        for r in rows:
            f.write("\t".join([r["tid"], r["reg"], r["grp"], r["name"], r["orig"]]) + "\n")
    print("wrote", out, len(rows), "titles")


if __name__ == "__main__":
    main(*sys.argv[1:2])
