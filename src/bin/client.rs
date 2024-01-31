use std::fs::File;
use std::io::Write;

use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::TcpStream;

#[derive(Debug)]
struct RequestData {
    data: Vec<u8>,
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("hello from the client!");
    let input_file_arg = std::env::args().nth(1).unwrap();
    let number_idx = input_file_arg.find(|c: char| c.is_numeric()).unwrap();
    let number = input_file_arg.chars().nth(number_idx).unwrap();

    let stream = TcpStream::connect("0.0.0.0:1234").await.unwrap();
    let (mut read, mut write) = stream.into_split();

    let sender_handle = tokio::spawn(async move {
        let req_file = tokio::fs::File::open(input_file_arg).await.unwrap();
        let mut reader = tokio::io::BufReader::new(req_file);
        let mut buffer = Vec::new();

        // Read the entire file into a buffer
        reader.read_to_end(&mut buffer).await.unwrap();
        let mut send_count = 0;

        // Split the buffer into lines and send each line as a request
        for (idx, line) in buffer.split(|&c| c == b'\n' || c == b'\r').enumerate() {
            // dbg!(&line);
            if idx == 0 || line.is_empty() {
                continue;
            }
            let mut data = line.to_vec();
            data.push(b'\n');
            let request = RequestData { data };
            write.write_all(&request.data).await.unwrap();
            send_count += 1;
        }
        // sleep(Duration::from_secs(1)).await;
        println!("send_count: {}", send_count);
    });

    let receiver_handle = tokio::spawn(async move {
        let mut output_file = File::create(format!("testcases/output_{}.csv", number)).unwrap();
        println!("Writing to output file: output_{}.csv", number);
        write!(
            output_file,
            "game_start_time, game_end_time, court_number, list_of_player_ids\r\n"
        )
        .unwrap();

        let mut buffer = [0; 1024];
        let mut recv_count = 0;

        while let Ok(bytes_read) = read.read(&mut buffer).await {
            println!("bytes_read: {}", bytes_read);
            if bytes_read == 0 {
                break;
            }

            let response_str = String::from_utf8_lossy(&buffer[..bytes_read]);
            println!("Received: {}", response_str);
            // write!(output_file, "{}", response_str).unwrap();
            output_file.write_all(&buffer[..bytes_read]).unwrap();
            recv_count += 1;
        }

        println!("recv_count: {}", recv_count);
    });

    let _ = tokio::join!(sender_handle, receiver_handle);

    Ok(())
}
