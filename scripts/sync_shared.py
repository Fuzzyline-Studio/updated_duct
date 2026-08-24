"""
매장별 폴더(updated_duct_1/2/3)로 공용 파일을 복사해 동기화한다.
이 레포는 git core.symlinks=false라 심볼릭 링크를 못 쓰므로, 루트를
원본(source of truth)으로 두고 매번 이 스크립트로 덮어써서 드리프트를 막는다.

루트를 고친 뒤에는 반드시 이 스크립트를 실행할 것.
"""
import os
import shutil

_BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# 완전 공용 파일: 1/2/3호점 전부에 그대로 복사
FULLY_SHARED = ["library_and_pin.h", "game_state.ino", "timer.ino"]

# 부분 공용 파일: 로직이 갈리지 않은 매장에만 복사 (1호점은 자체 포크본 유지, 덮어쓰지 않음)
PARTIALLY_SHARED = ["sensor.ino", "updated_duct_function.ino"]
PARTIALLY_SHARED_TARGETS = ["updated_duct_2", "updated_duct_3"]

ALL_SITE_FOLDERS = ["updated_duct_1", "updated_duct_2", "updated_duct_3"]


def _copy(filename, target_folder):
    src = os.path.join(_BASE_DIR, filename)
    dst_dir = os.path.join(_BASE_DIR, target_folder)
    if not os.path.isdir(dst_dir):
        print(f"  스킵: {target_folder} 폴더가 없음")
        return
    dst = os.path.join(dst_dir, filename)
    shutil.copy2(src, dst)
    print(f"  {filename} -> {target_folder}/")


def main():
    print("공용 파일 동기화 시작 (루트 -> 매장 폴더)")

    print("\n[완전 공용]")
    for filename in FULLY_SHARED:
        for folder in ALL_SITE_FOLDERS:
            _copy(filename, folder)

    print("\n[부분 공용] (updated_duct_1은 자체 포크본 유지, 건너뜀)")
    for filename in PARTIALLY_SHARED:
        for folder in PARTIALLY_SHARED_TARGETS:
            _copy(filename, folder)

    print("\n완료. updated_duct_1의 sensor.ino / updated_duct_function.ino는 수동으로만 관리하세요.")


if __name__ == "__main__":
    main()
