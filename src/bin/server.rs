use std::collections::VecDeque;

use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::{TcpListener, TcpStream};

#[derive(Debug, Clone)]
struct Player {
    player_id: usize,
    time: usize,
    gender: Gender,
    pref: Pref,
}

impl Player {
    fn new(player_id: &str, time: &str, gender: &str, pref: &str) -> Self {
        let player_id = player_id.parse::<usize>().unwrap();
        let time = time.parse::<usize>().unwrap();
        let gender = match gender {
            "M" => Gender::M,
            "F" => Gender::F,
            _ => panic!("Invalid Request [gender]"),
        };
        let pref = match pref {
            "S" => Pref::S,
            "D" => Pref::D,
            "b" => Pref::SD,
            "B" => Pref::DS,
            _ => panic!("Invalid Request [preference]"),
        };
        Player {
            player_id,
            time,
            gender,
            pref,
        }
    }
}

#[derive(Debug, PartialEq, Clone)]
enum Gender {
    M,
    F,
}

#[derive(Debug, PartialEq, Clone)]
enum Pref {
    S,
    D,
    SD,
    DS,
}

#[derive(Debug)]
struct GameDuration {
    pref: Pref,
    duration: [usize; 2], // [M, F]
}

#[derive(Debug)]
struct Court {
    id: usize,
    bookings: Vec<(usize, usize)>,
}

fn allocate_court(courts: &mut Vec<Court>, game_start_time: usize, game_end_time: usize) -> usize {
    // find free court using rust Range
    let mut court_id = 0;
    for (idx, court) in courts.iter().enumerate() {
        let mut is_free = true;
        for (start_time, end_time) in &court.bookings {
            if game_start_time < *end_time && game_end_time > *start_time {
                is_free = false;
                break;
            }
        }
        if is_free {
            court_id = idx;
            break;
        }
    }

    let court = &mut courts[court_id];
    court.bookings.push((game_start_time, game_end_time));
    court.id
}

async fn process_request(
    _stream: &mut TcpStream,
    buffer: &[u8],
    single_players: &mut VecDeque<Player>,
    doubles_players: &mut VecDeque<Player>,
    courts: &mut Vec<Court>,
    game_durations: &Vec<GameDuration>,
) -> Vec<Vec<u8>> {
    let request_str = String::from_utf8_lossy(buffer);
    // println!("request: {}", request_str);

    let split_request: Vec<&str> = request_str.split(',').collect();
    // println!("split request: {:?}", split_request);

    let [player_id, time, gender, pref] = split_request[..] else {
        panic!("Invalid Request")
    };
    let player = Player::new(player_id, time, gender, pref);
    // println!("player: {:?}", player);

    match player.pref {
        Pref::S => single_players.push_back(player),
        Pref::D => doubles_players.push_back(player),
        Pref::SD => single_players.push_back(player),
        Pref::DS => single_players.push_back(player),
    }

    let mut responses = Vec::new();
    while single_players.len() >= 2 {
        let mut players = Vec::new();
        for _ in 0..2 {
            players.push(single_players.pop_front().unwrap());
        }

        let game_start_time = players.iter().map(|p| p.time).max().unwrap();
        let male_player = players.iter().find(|p| p.gender == Gender::M);
        let game_end_time = male_player
            .map_or(game_start_time + game_durations[0].duration[1], |_| {
                game_start_time + game_durations[0].duration[0]
            });

        let court_id = allocate_court(courts, game_start_time, game_end_time);

        responses.push(format!(
            "{},{},{},{},{}\r\n",
            game_start_time, game_end_time, court_id, players[0].player_id, players[1].player_id,
        ));
    }

    while doubles_players.len() >= 4 {
        let mut players = Vec::new();
        for _ in 0..4 {
            players.push(doubles_players.pop_front().unwrap());
        }

        let game_start_time = players.iter().map(|p| p.time).max().unwrap();
        let male_player = players.iter().find(|p| p.gender == Gender::M);
        let game_end_time = male_player
            .map_or(game_start_time + game_durations[1].duration[1], |_| {
                game_start_time + game_durations[1].duration[0]
            });
        let court_id = allocate_court(courts, game_start_time, game_end_time);

        responses.push(format!(
            "{},{},{},{},{},{},{}\r\n",
            game_start_time,
            game_end_time,
            court_id,
            players[0].player_id,
            players[1].player_id,
            players[2].player_id,
            players[3].player_id
        ));
    }

    let responses: Vec<Vec<u8>> = responses
        .into_iter()
        .map(|s| s.as_bytes().to_vec())
        .collect();
    responses

    // let mut response = String::new();

    // if player_id.parse::<usize>().unwrap() % 2 == 0 {
    //     response.push_str("1,11,1,1,2");

    //     response.push('\r');
    //     response.push('\n');
    // }

    // response.as_bytes().to_vec()
}

async fn process_remaining_players(
    stream: &mut TcpStream,
    single_players: &mut VecDeque<Player>,
    doubles_players: &mut VecDeque<Player>,
    courts: &mut Vec<Court>,
    game_durations: &Vec<GameDuration>,
) -> Vec<Vec<u8>> {
    let mut responses = Vec::new();

    // let is_doubles_a_option = if single_players.len() > 0 {
    //     single_players[0].pref == Pref::SD
    // } else {
    //     false
    // };
    // if is_doubles_a_option {
    //     let single_pref_player_index = doubles_players.iter().position(|p| p.pref == Pref::DS);
    //     if let Some(index) = single_pref_player_index {
    //         let player = doubles_players.remove(index).unwrap();
    //         single_players.push_back(player);
    //     }
    // }

    let mut remaining_players = Vec::new();
    while let Some(player) = single_players.pop_front() {
        remaining_players.push(player);
    }
    while let Some(player) = doubles_players.pop_front() {
        remaining_players.push(player);
    }

    remaining_players.sort_by(|a, b| a.time.cmp(&b.time));
    println!("remaining_players: {:?}", remaining_players);

    let mut single_players: VecDeque<Player> = VecDeque::new();
    remaining_players
        .iter()
        .filter(|p| p.pref == Pref::S || p.pref == Pref::SD || p.pref == Pref::DS)
        .for_each(|p| single_players.push_back(p.clone()));
    let single = single_players.len();

    let mut doubles_players: VecDeque<Player> = VecDeque::new();
    remaining_players
        .iter()
        .filter(|p| p.pref == Pref::D || p.pref == Pref::SD || p.pref == Pref::DS)
        .for_each(|p| doubles_players.push_back(p.clone()));
    let doubles = doubles_players.len();

    match (single, doubles) {
        // (0,0)
        // (0,1)
        // (0,2)
        // (0,3)
        // (0,4) x
        // (1,0)
        // (1,1)
        // (1,2)
        // (1,3)
        (1, 4) => single_players.pop_front().unwrap(),
        // (2,0) x
        // (2,1)
        // (2,2)
        // (2,3)
        (2, 4) => doubles_players.pop_front().unwrap(),
        // (3,0) x
        // (3,1) x
        // (3,2)
        // (3,3)
        (3, 4) => doubles_players.pop_front().unwrap(),
        // (4,0) x
        // (4,1) x
        // (4,2) x
        // (4,3)
        (4, 4) => doubles_players.pop_front().unwrap(),
        _ => Player {
            player_id: 0,
            time: 0,
            gender: Gender::M,
            pref: Pref::S,
        },
    };

    while single_players.len() >= 2 {
        let mut players = Vec::new();
        for _ in 0..2 {
            players.push(single_players.pop_front().unwrap());
        }

        let game_start_time = players.iter().map(|p| p.time).max().unwrap();
        let male_player = players.iter().find(|p| p.gender == Gender::M);
        let game_end_time = male_player
            .map_or(game_start_time + game_durations[0].duration[1], |_| {
                game_start_time + game_durations[0].duration[0]
            });

        let court_id = allocate_court(courts, game_start_time, game_end_time);

        responses.push(format!(
            "{},{},{},{},{}\r\n",
            game_start_time, game_end_time, court_id, players[0].player_id, players[1].player_id,
        ));
    }

    while doubles_players.len() >= 4 {
        let mut players = Vec::new();
        for _ in 0..4 {
            players.push(doubles_players.pop_front().unwrap());
        }

        let game_start_time = players.iter().map(|p| p.time).max().unwrap();
        let male_player = players.iter().find(|p| p.gender == Gender::M);
        let game_end_time = male_player
            .map_or(game_start_time + game_durations[1].duration[1], |_| {
                game_start_time + game_durations[1].duration[0]
            });
        let court_id = allocate_court(courts, game_start_time, game_end_time);

        responses.push(format!(
            "{},{},{},{},{},{},{}\r\n",
            game_start_time,
            game_end_time,
            court_id,
            players[0].player_id,
            players[1].player_id,
            players[2].player_id,
            players[3].player_id
        ));
    }

    println!("remaining single_players: {:?}", single_players);
    println!("remaining doubles_players: {:?}", doubles_players);

    let responses = responses
        .into_iter()
        .map(|s| s.as_bytes().to_vec())
        .collect();
    responses
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("hello from the server!");

    let listener = TcpListener::bind("0.0.0.0:1234")
        .await
        .expect("Failed to bind to address");

    while let Ok((stream, _)) = listener.accept().await {
        println!("\nnew client connected");
        println!("client addr: {:?}", stream.peer_addr());

        let mut singles_players = VecDeque::new();
        let mut doubles_players = VecDeque::new();
        let mut courts = Vec::new();
        for i in 1..5 {
            courts.push(Court {
                id: i,
                bookings: Vec::new(),
            });
        }
        let game_durations = vec![
            GameDuration {
                pref: Pref::S,
                duration: [10, 5],
            },
            GameDuration {
                pref: Pref::D,
                duration: [15, 10],
            },
        ];

        tokio::spawn(async move {
            let mut stream = stream;
            let mut buffer = [0; 1024];

            let mut count = 0;

            while let Ok(bytes_read) = stream.read(&mut buffer).await {
                println!("bytes_read: {}", bytes_read);
                if bytes_read == 0 {
                    break;
                }

                // split the buffer into lines and process each line as a request
                for line in buffer[..bytes_read].split(|&c| c == b'\n') {
                    if line.is_empty() {
                        continue;
                    }
                    let responses = process_request(
                        &mut stream,
                        line,
                        &mut singles_players,
                        &mut doubles_players,
                        &mut courts,
                        &game_durations,
                    )
                    .await;
                    for response in responses {
                        stream.write_all(response.as_slice()).await.unwrap();
                        count += 1;
                    }
                }
            }

            let responses = process_remaining_players(
                &mut stream,
                &mut singles_players,
                &mut doubles_players,
                &mut courts,
                &game_durations,
            )
            .await;
            for response in responses {
                stream.write_all(response.as_slice()).await.unwrap();
                count += 1;
            }
            println!("response count: {}", count);

            println!("client disconnected");
        });
    }

    Ok(())
}
