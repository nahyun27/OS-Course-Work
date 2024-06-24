import time

# 수정 비트를 사용하는 이유:
# 페이지를 수정하면 디스크 동기화가 필요하며, 디스크 쓰기는 비용이 많이 듭니다.
# 따라서 수정된 페이지는 가능한 한 교체되지 않도록 하여 디스크 쓰기 연산을 최소화합니다.
# 교체될 페이지는 00(유효하지 않음, 참조되지 않음) 또는 01(유효, 참조되지 않음)입니다.
# 01이면 디스크 쓰기를 수행하고, 00이면 디스크 쓰기를 하지 않고 교체합니다.

FRAME_SIZE = 3
VALID = 1 << 0
REFERENCE = 1 << 1
MODIFY = 1 << 2

class Frame:
    def __init__(self):
        self.number = -1
        self.bits = 0

# 전역 변수 초기화
front = 0
tail = 0
queue_length = 0
hit_count = 0
fault_count = 0

def simulate_disk_io():
    """디스크 쓰기/읽기 시뮬레이션"""
    time.sleep(0.01 / 100)

def add_page(frame, page_number):
    """페이지를 큐에 추가하는 함수"""
    global front, tail, queue_length

    if queue_length < FRAME_SIZE:
        frame[tail].number = page_number
        frame[tail].bits = VALID
        tail = (tail + 1) % FRAME_SIZE
        queue_length += 1
    else:
        while True:
            if frame[front].bits & REFERENCE:
                frame[front].bits &= ~REFERENCE
                front = (front + 1) % FRAME_SIZE
            else:
                simulate_disk_io()
                frame[front].number = page_number
                frame[front].bits |= VALID
                front = (front + 1) % FRAME_SIZE
                break

def add_page_enhanced(frame, page_number, is_modified):
    """강화된 페이지 교체 알고리즘으로 페이지를 큐에 추가하는 함수"""
    global front, tail, queue_length

    if queue_length < FRAME_SIZE:
        frame[tail].number = page_number
        frame[tail].bits = VALID | (MODIFY if is_modified else 0)
        tail = (tail + 1) % FRAME_SIZE
        queue_length += 1
    else:
        attempts = 0
        while True:
            current_bits = frame[front].bits
            if current_bits & REFERENCE:
                frame[front].bits &= ~REFERENCE
                front = (front + 1) % FRAME_SIZE
            elif current_bits & MODIFY:
                if attempts >= FRAME_SIZE * 2:
                    simulate_disk_io()
                    frame[front].number = page_number
                    frame[front].bits = VALID | (MODIFY if is_modified else 0)
                    front = (front + 1) % FRAME_SIZE
                    break
                front = (front + 1) % FRAME_SIZE
            else:
                frame[front].number = page_number
                frame[front].bits = VALID | (MODIFY if is_modified else 0)
                front = (front + 1) % FRAME_SIZE
                break
            attempts += 1

def access_page(frame, page_number, enhanced):
    """페이지를 참조하는 함수"""
    global hit_count, fault_count

    for i in range(FRAME_SIZE):
        if (frame[i].bits & VALID) and (frame[i].number == page_number):
            frame[i].bits |= REFERENCE
            hit_count += 1
            return
    if enhanced:
        add_page_enhanced(frame, page_number, False)
    else:
        add_page(frame, page_number)
    fault_count += 1

def modify_page(frame, page_number):
    """페이지를 수정하는 함수"""
    global hit_count, fault_count

    for i in range(FRAME_SIZE):
        if (frame[i].bits & VALID) and (frame[i].number == page_number):
            frame[i].bits |= (REFERENCE | MODIFY)
            hit_count += 1
            return
    add_page_enhanced(frame, page_number, True)
    fault_count += 1

def main():
    global front, tail, queue_length, hit_count, fault_count

    frame = [Frame() for _ in range(FRAME_SIZE)]
    pages = [7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1]
    modifications = [0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0]

    hit_count = fault_count = 0
    start_time = time.time()

    for i in range(20):
        access_page(frame, pages[i], False)

    elapsed_time = time.time() - start_time
    print(f"기본 알고리즘 - Hit 횟수: {hit_count}")
    print(f"기본 알고리즘 - Fault 횟수: {fault_count}")
    print(f"기본 알고리즘 - 경과 시간: {elapsed_time:.9f} 초")

    # 프레임 초기화
    front = 0
    tail = 0
    queue_length = 0
    hit_count = fault_count = 0
    for i in range(FRAME_SIZE):
        frame[i].bits = 0
        frame[i].number = -1

    start_time = time.time()

    for i in range(20):
        if modifications[i]:
            modify_page(frame, pages[i])
        else:
            access_page(frame, pages[i], True)

    elapsed_time = time.time() - start_time
    print(f"강화된 알고리즘 - Hit 횟수: {hit_count}")
    print(f"강화된 알고리즘 - Fault 횟수: {fault_count}")
    print(f"강화된 알고리즘 - 경과 시간: {elapsed_time:.9f} 초")

if __name__ == "__main__":
    main()
