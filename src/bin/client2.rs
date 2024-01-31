use std::fs::File;
use std::io::Write;

use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::TcpStream;
use tokio::select;
use tokio::sync::mpsc;

#[derive(Debug)]
struct RequestData {
    data: Vec<u8>,
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("hello from the client!");
    let input_file_arg = std::env::args().nth(1).unwrap();
    let number_idx = input_file_arg.find(|c: char| c.is_numeric()).unwrap();
    println!("number_idx: {}", number_idx);
    let number = input_file_arg.chars().nth(number_idx).unwrap();
    println!("number: {}", number);

    let mut stream = TcpStream::connect("0.0.0.0:1234").await.unwrap();
    let mut stream = stream.into_split();
    let (mut read, mut write) = stream;

    let write_handle = tokio::spawn(async move {
        write.write(b"2,1,F,S\n").await.unwrap();
    });

    let mut buffer = [0; 1024];
    let mut output_file = File::create(format!("output_{}.csv", number)).unwrap();
    write!(
        output_file,
        "game_start_time, game_end_time, court_number, list_of_player_ids\r\n"
    )
    .unwrap();

    let receiver_handle = tokio::spawn(async move {
        while let Ok(bytes_read) = read.read(&mut buffer).await {
            println!("bytes_read: {}", bytes_read);
            if bytes_read == 0 {
                break;
            }

            // let response_str = u8_array_to_string(&buffer[..bytes_read]);
            println!("response: {:?}", buffer);
        }
    });

    // write_handle.await.unwrap();

    select! {
        _ = write_handle => {
            println!("write_handle finished");
        }
        _ = receiver_handle => {
            println!("receiver_handle finished");
        }
    }

    Ok(())
}
