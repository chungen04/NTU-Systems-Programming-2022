#!/bin/sh

sample_1() {
    ./main 3 6 40 -1
}

sample_2() {
    ./main 3 5 3 -1 &
    child=$!
    sleep 1.5
    kill -TSTP $child
    sleep 5
    kill -TSTP $child
    wait $child
}

sample_3() {
    ./main 3 4 -1 2 &
    child=$!
    sleep 0.5
    exec 3>2_max_subarray
    kill -TSTP $child
    sleep 1
    echo '  -1' >&3
    sleep 5
    echo '  99' >&3
    wait $child
}

sample_4() {
    ./main 3 -1 -1 6 &
    child=$!
    sleep 0.5
    exec 3>2_max_subarray
    kill -TSTP $child
    sleep 1
    echo '   1' >&3
    sleep 1
    echo '   0' >&3
    sleep 1
    echo '8848' >&3
    sleep 1
    echo '  -5' >&3
    sleep 1
    echo ' 236' >&3
    sleep 1
    echo '   4' >&3
    sleep 1
    wait $child
}

print_help() {
    echo "usage: $0 [subtask]"
}

main() {
    case "$1" in
        1)
            sample_1
            ;;
        2)
            sample_2
            ;;
        3)
            sample_3
            ;;
        4)
            sample_4
            ;;
        *)
            print_help
            ;;
    esac
}

main "$1"