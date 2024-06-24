def FIFO(size, pages):
    SIZE = size
    memory = []
    faults = 0
    history = []

    for page in pages:
        if memory.count(page) == 0 and len(memory) < SIZE:
            memory.append(page)
            faults += 1
        elif memory.count(page) == 0 and len(memory) == SIZE:
            memory.pop(0)
            memory.append(page)
            faults += 1
        history.append(memory[:])

    return faults, history

def LRU(size, pages):
    SIZE = size
    memory = []
    faults = 0
    history = []

    for page in pages:
        if memory.count(page) == 0 and len(memory) < SIZE:
            memory.append(page)
            faults += 1
        elif memory.count(page) == 0 and len(memory) == SIZE:
            memory.pop(0)
            memory.append(page)
            faults += 1
        elif memory.count(page) > 0:
            memory.remove(page)
            memory.append(page)
        history.append(memory[:])

    return faults, history

def OPT(size, pages):
    SIZE = size
    memory = []
    faults = 0
    history = []

    for i, page in enumerate(pages):
        if page not in memory:
            if len(memory) < SIZE:
                memory.append(page)
            else:
                # Find the page with the farthest next use
                future_uses = []
                for mem_page in memory:
                    if mem_page in pages[i+1:]:
                        future_uses.append(pages[i+1:].index(mem_page))
                    else:
                        future_uses.append(float('inf'))
                memory.pop(future_uses.index(max(future_uses)))
                memory.append(page)
            faults += 1
        history.append(memory[:])

    return faults, history

# 예제 사용법
pages = [7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1]
page_frame_size = 3

# FIFO
fifo_faults, fifo_history = FIFO(page_frame_size, pages)
print("\n===============FIFO=================")
for state in fifo_history:
    print(state)
print(f"{fifo_faults} page faults.")

# LRU
lru_faults, lru_history = LRU(page_frame_size, pages)
print("\n================LRU=================")
for state in lru_history:
    print(state)
print(f"{lru_faults} page faults.")

# OPT
opt_faults, opt_history = OPT(page_frame_size, pages)
print("\n================OPT=================")
for state in opt_history:
    print(state)
print(f"{opt_faults} page faults.")
