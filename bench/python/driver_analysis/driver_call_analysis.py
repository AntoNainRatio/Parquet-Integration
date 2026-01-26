from collections import defaultdict
import statistics

log_file = r"driver_analysis/driver_call_timed.txt"

data = defaultdict(list)

# Lecture du fichier
with open(log_file, "r", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue
        name, value = line.split(": ")
        data[name.strip()].append(int(value.strip()))

total = 0
# Calcul et affichage des statistiques
for func, times in data.items():
    total += sum(times)

# Calcul et affichage des statistiques
for func, times in data.items():
    func_total = sum(times)
    mean = statistics.mean(times)
    variance = statistics.variance(times) if len(times) > 1 else 0
    stdev = statistics.stdev(times) if len(times) > 1 else 0

    print(f"{func}")
    print(f"  Appels               : {len(times)}")
    print(f"  Total (ms)           : {func_total}")
    print(f"  Portion du total     : {(func_total/total) * 100:.2f} %")
    print(f"  Moyenne (ms)         : {mean:.2f}")
    print(f"  Variance             : {variance:.2f}")
    print(f"  Écart-type           : {stdev:.2f}")
    print(f"  Min / Max            : {min(times)} / {max(times)}")
    print()

print(f"Total (ms) : {total}")