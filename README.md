# Tennis court booking system

to facilitate the concepts learning of parallel and distributed systems

to facilitate the concepts learning of parallel and distributed systems

## run

```bash
cargo run --bin server # server
cargo run --bin client testcases/test_case_1.csv # client
```

## generating test cases

```bash
for i in {2..10}; do
    ./seed_test_case.py testcases/test_case_$i.csv
done
```

## checking on test cases

```bash
cargo run --bin server # first start the server
for i in {1..10}; do
    cargo run --bin client testcases/test_case_$i.csv
done

# testcases/output_$i.csv will be generated
```

Output file will be output_[number from test case file].csv. For example, output_1.csv, output_2.csv.
