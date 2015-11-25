QEMUNAME = "qemu-system-x86"

Given /^the root path is "(.*?)"$/ do |url|
  Capybara.app_host = url
end

When /^I am on the root page$/ do
    visit '/'
end

When /^I visit "(.*?)"$/ do |url|
    visit url
end

Then /^I should not see "(.*?)"$/ do |text|
    page.should have_no_content text
end

Then /^I should see "(.*?)"$/ do |text|
    page.should have_content text 
end

When /^I search for "(.*?)"$/ do |keyword|
  fill_in 'sb_form_q', :with => keyword
  click_button 'sb_form_go'
end

Then /^show me the page$/ do
  save_and_open_page
end

Before('@multi') do |scenario|
  if (!system("ps -eo comm|grep -i #{QEMUNAME}"))     # Added by huangye 151008
     boot_linux("")
     run_cmd("httpd")
  end 
end

After('@multi') do |scenario|
  # system("pkill -9 #{QEMUNAME}");     # Added by huangye 151008
  # sleep(2)
  p "#{scenario.name} end"
end

