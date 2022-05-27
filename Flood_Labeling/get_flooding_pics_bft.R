#--------- Source the secrets ----------
# Change to your path for this file
# Check in the teams filesystem at `SunnyD Flood Sensor Network - Documents/Web and database/Keys/sunnyday_postgres_keys.R`
source("/Users/adam/Library/CloudStorage/OneDrive-SharedLibraries-UniversityofNorthCarolinaatChapelHill/SunnyD Flood Sensor Network - Documents/Web and database/Keys/sunnyday_postgres_keys.R")

#------- Packages ---------
library(googledrive)
library(pool)
library(RPostgres)
library(tidyverse)
library(httr)
library(jsonlite)
library(lubridate)
library(foreach)
library(progressr)
library(data.table)

#--------- Load google authentications -----------
# You can either use the GOOGLE_JSON_KEY which is for the service account that handles the photo API (Probably OK to do, but might cause photo API to stop for awhile)
# OR you can uncomment theother `drive_auth` line and sign in via the browser to a google account with adequate folder permission
googledrive::drive_auth(path = Sys.getenv("GOOGLE_JSON_KEY"))
# googledrive::drive_auth()

#--------- Functions ------------
flood_counter <- function(dates, start_number = 0, lag_hrs = 8){
  
  lagged_time <- difftime(dates, dplyr::lag(dates), units = "hours") %>% 
    replace_na(duration(0))
  
  lead_time <- difftime(dplyr::lead(dates),dates, units = "hours") %>% 
    replace_na(duration(0))
  
  group_change_vector <- foreach(i = 1:length(dates), .combine = "c") %do% {
    x <- 0
    
    if(abs(lagged_time[i]) > hours(lag_hrs)){
      x <- 1
    }
    
    x
  }
  
  group_vector <- cumsum(group_change_vector) + 1 + start_number
  return(group_vector)
}

# This function downloads data from the API. The username and password are secret, so pls don't share!
# This code took my computer 55 seconds

get_sunnyd_data <- function(sensor_ID = "BF_01", total_min_date, total_max_date){
  total_days <- lubridate::time_length(total_max_date - total_min_date, unit = "days")
  handlers(global = TRUE)
  p <- progressor(total_days)
  cat("Downloading data from the SunnyD API... ","\n")
  
  sunnyd_data <- foreach(i = 1:total_days, .combine = "bind_rows") %do% {
    request <- httr::GET(url = "https://api-sunnydayflood.apps.cloudapps.unc.edu/get_water_level",
                         query = list(
                           min_date = format(total_min_date + days(i - 1), "%Y-%m-%d"),
                           max_date = format(total_min_date + days(i), "%Y-%m-%d"),
                           sensor_ID = sensor_ID
                         ),
                         authenticate(Sys.getenv("API_USERNAME"), Sys.getenv("API_PASSWORD")))
    
    
    if(jsonlite::fromJSON(rawToChar(request$content)) %>% length(.) > 0){
      data <- jsonlite::fromJSON(rawToChar(request$content)) %>% 
        as_tibble() %>% 
        mutate(date = lubridate::ymd_hms(date),
               date_surveyed = lubridate::ymd_hms(date_surveyed))
      p()
      return(data)
    }
    
    p()
    return()
  }
  
  cat("Here is your data, human. Sincerely, 🤖","\n")
  return(sunnyd_data)
}

get_flooding_pics <- function(camera_ID = "CAM_BF_01", path_to_put_pics, flood_events, download = T){
  # Get the unique flood events list
  flood_event_names <- flood_events$flood_event |> unique()
  
  # create a progress bar that is as long as the max flood event name
  p2 <- progressor(max(flood_event_names))
  cat("Downloading pics for", camera_ID,"from Google Drive...","\n")
  
  flood_event_pics <- foreach(i = flood_event_names, .combine = "bind_rows") %do% {
    
    # select a flood event and make df
    selected_flood <- flood_events %>%
      filter(flood_event == i)
    
    # get the date of the flooding
    days_of_flood <- seq.Date(from = selected_flood$start_date, to = selected_flood$end_date, by = "days")
    
    # For each day of flooding, get info about the folder for that day,
    # Get a list of images within that folder,
    # Parse out the time each photo was taken by looking at the name of the file
    # Select only photos within the time period of the flood event (+- 5 minutes on either side)
    # Add a few details about the picture to the df
    # Download the selected photos to the folder `path_to_put_pics`
    pics_df <- foreach(j = 1:length(days_of_flood), .combine = "bind_rows") %do% {
      with_drive_quiet(
        folder_info <- googledrive::drive_get(path = paste0("Images/",camera_ID,"/",days_of_flood[j],"/"),shared_drive = as_id(Sys.getenv("GOOGLE_SHARED_DRIVE_ID")))
      )
      
      if(nrow(folder_info) == 0){
        return()
      }
      
      image_list <- foreach(l = 1:nrow(folder_info), .combine = "bind_rows") %do% {
        with_drive_quiet(drive_ls(folder_info$id[l])) %>%
          mutate(pic_time = ymd_hms(sapply(stringr::str_split(name, pattern = "_"), tail, 1)))
      }
      
      filtered_image_list <- image_list |> 
        filter(pic_time >= (selected_flood$start_dttm - minutes(5)) & pic_time <= (selected_flood$end_dttm + minutes(5))) |> 
        mutate(flood_event = i) |> 
        mutate(pic_view_link = drive_resource[[1]]$webViewLink,
               pic_download_link = drive_resource[[1]]$webContentLink)
      
      if(download == T){
        pic_dribbles <- purrr::map(.x = 1:nrow(filtered_image_list), .f = function(x){
          with_drive_quiet(
            googledrive::drive_download(filtered_image_list[x,], path = paste0(my_pic_path, filtered_image_list$name[x]))
          )
        }) 
      }
      
      data.table::rbindlist(pic_dribbles) |> as_tibble()
      
    }
    p2()
    pics_df
  }
}

#------- Download data from SunnyD API - be patient pls! ------------

# Set the min and max dates we want to download
total_min_date <- lubridate::ymd(20210622) #2021-06-22
total_max_date <- lubridate::ymd(20220322) #2022-03-22

# `date` is UTC, `road_wl` is water level relative to the road in FEET
data <- get_sunnyd_data(sensor_ID = "BF_01", total_min_date, total_max_date)

sunnyd_data <- data |> 
  dplyr::select(sensor_ID, date, road_wl =road_water_level_adj)

# Select the flooding measurements, when the water level was above the road
# Group the flooding measurements into flood events using the function `flood_counter`
flooding_measurements <- sunnyd_data |> 
  filter(road_wl > 0) |> 
  mutate(flood_event = flood_counter(dates = date,lag_hrs = 2))
      
# Plot to see the road water level and the flood events in colors
sunnyd_data |> 
  ggplot()+
  geom_line(aes(x=date, y = road_wl), color="darkgrey")+
  geom_point(data = flooding_measurements, aes(x=date,y=road_wl,color=factor(flood_event)))+
  theme_bw()

# Summarize so each flood event is one row
flood_events <- flooding_measurements |> 
  group_by(flood_event) |> 
  summarize(number_of_measurements = n(),
            start_dttm = min(date, na.rm=T),
            start_date = start_dttm |> as_date(),
            end_dttm = max(date, na.rm=T),
            end_date = end_dttm |> as_date(),
            max_wl = max(road_wl, na.rm=T)) 


# Define the path where you want all the pictures downloaded. Make sure you have the trailing slash like below
my_pic_path <- "/Users/adam/Downloads/drive_test/"

# Download all the pictures! This will take awhile...
pic_info_df <- get_flooding_pics(path_to_put_pics = my_pic_path, 
                                 flood_events = flood_events,
                                 download = T)
  
# # You can always join `flood_events` to this pic_info_df and filter by max_wl to sort pictures by max wl (flood event average)
# pic_info_df |> 
#   left_join(flood_events, by = "flood_event")

# If you save as a RDS object, you can load it easily back int R and access all of the info contained in the drive_resource column. This column is a list and won't be written in a csv
pic_info_df |> 
  write_rds("/Users/adam/Downloads/pic_info.rds")

# Write a CSV for good measure
pic_info_df |> 
  write_csv("/Users/adam/Downloads/pic_info.csv")
