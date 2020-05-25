#
# Be sure to run `pod lib lint SoundHoleCore.podspec' to ensure this is a
# valid spec before submitting.
#
# Any lines starting with a # are optional, but their use is encouraged
# To learn more about a Podspec see https://guides.cocoapods.org/syntax/podspec.html
#

Pod::Spec.new do |s|
	s.name             = 'SoundHoleCore'
	s.version          = '0.1.0'
	s.summary          = 'An audio streaming library'

# This description is used to generate tags and improve search results.
#   * Think: What does it do? Why did you write it? What is the focus?
#   * Try to keep it short, snappy and to the point.
#   * Write the description between the DESC delimiters below.
#   * Finally, don't worry about the indent, CocoaPods strips it!

	s.description      = <<-DESC
	An audio streaming library with a variety of sources.
							DESC

	s.homepage         = 'https://github.com/lufinkey/SoundHoleCore'
	# s.screenshots     = 'www.example.com/screenshots_1', 'www.example.com/screenshots_2'
	# s.license          = { :type => 'MIT', :file => 'LICENSE' }
	s.author           = { 'Luis Finke' => 'luisfinke@gmail.com' }
	s.source           = { :git => 'https://github.com/lufinkey/SoundHoleCore.git', :tag => s.version.to_s }
	s.social_media_url = 'https://twitter.com/lufinkey'

	s.ios.deployment_target = '12.0'
	s.osx.deployment_target = '10.14'

	s.static_framework = true

	s.source_files = 'src/soundhole/**/*.{c,cc,cpp,m,mm,h,hpp}', 'external/cxxurl/url.cpp', 'external/json11/json11.cpp'
	s.ios.frameworks = 'SpotifyAudioPlayback'

	s.public_header_files = 'src/soundhole/**/*.{h,hpp}', 'external/cxxurl/*.{h,hpp}', 'external/json11/*.{h,hpp}'
	s.preserve_paths = 'external/**/*', 'src/soundhole/scripts/**/*', 'tools/**/*'
	s.header_mappings_dir = 'src/soundhole'
	s.pod_target_xcconfig = {
		'HEADER_SEARCH_PATHS' => [
			"$(PODS_ROOT)/SoundHoleCore/src",
			"$(PODS_ROOT)/SoundHoleCore/src/soundhole/scripts/js/build",
			"$(PODS_ROOT)/DataCpp/src",
			"$(PODS_ROOT)/AsyncCpp/src",
			"$(PODS_ROOT)/IOCpp/src",
			"$(PODS_ROOT)/NodeJSEmbed/src",
			"$(PODS_ROOT)/NodeJSEmbed/external/nodejs/build/addon-api",
			"$(PODS_ROOT)/SoundHoleCore/external"
		],
		'CLANG_CXX_LANGUAGE_STANDARD' => 'gnu++17'
	}
	s.ios.pod_target_xcconfig = {
		'HEADER_SEARCH_PATHS' => [
			"$(PODS_ROOT)/NodeJSEmbed/external/nodejs/build/mobile/include",
			"$(PODS_ROOT)/NodeJSEmbed/external/nodejs/build/mobile/include/nodejs",
			"$(PODS_ROOT)/NodeJSEmbed/external/nodejs/build/mobile/node/deps/openssl/openssl/include"
		],
		'FRAMEWORK_SEARCH_PATHS' => "${PODS_ROOT}/SoundHoleCore/external/spotify-ios-streaming-sdk"
	}
	s.user_target_xcconfig = {
		'HEADER_SEARCH_PATHS' => [
			"$(PODS_ROOT)/SoundHoleCore/src",
			"$(PODS_ROOT)/SoundHoleCore/external"
		]
	}
	s.ios.user_target_xcconfig = {
		'FRAMEWORK_SEARCH_PATHS' => "${PODS_ROOT}/SoundHoleCore/external/spotify-ios-streaming-sdk"
	}
	# s.frameworks = 'UIKit', 'MapKit'
	s.dependency 'DataCpp' # git@github.com:lufinkey/data-cpp.git
	s.dependency 'AsyncCpp' # git@github.com:lufinkey/async-cpp.git
	s.dependency 'IOCpp' # git@github.com:lufinkey/io-cpp.git
	s.dependency 'NodeJSEmbed' # git@github.com:lufinkey/nodejs-embed.git
	s.dependency 'sqlite3'
	
	s.prepare_command = "./tools/fetch_shared_dependencies.sh json11 cxxurl && ./tools/fetch_header_dependencies.sh && ./src/soundhole/scripts/build.sh && rm -rf src/soundhole/scripts/js/node_modules"
end
