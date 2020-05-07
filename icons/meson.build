public_icons = [ 'poi-airport.svg',
                 'poi-building.svg',
                 'poi-bus-stop.svg',
                 'poi-car.svg',
                 'poi-marker.svg',
                 'poi-railway-station.svg',
                 'poi-town.svg',
                 'poi-school.svg',
                 'poi-place-of-worship.svg',
                 'poi-restaurant.svg',
                 'poi-bar.svg',
                 'poi-light-rail-station.svg' ]

foreach icon : public_icons
    file = join_paths('maki' , icon)
    install_dir = join_paths(get_option('datadir'),
                             'icons',
                             'hicolor',
                             'scalable',
                             'places')

    install_data(file, install_dir: install_dir)
endforeach
