require 'capybara'
require 'capybara/cucumber'
require 'capybara/session'
#require 'capybara/mechanize/cucumber'
#Capybara.default_driver = :mechanize
# To resolve the problem:
#   no driver called :mechanize was found, available drivers: :rack_test, :selenium (Capybara::DriverNotFoundError)
#
# Add below line:
Capybara.default_driver = :selenium
