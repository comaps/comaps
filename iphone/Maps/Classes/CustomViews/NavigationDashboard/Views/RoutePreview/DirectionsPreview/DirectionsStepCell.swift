final class DirectionsStepCell: UITableViewCell {
    
    private let iconView: UIImageView = {
        let iv = UIImageView()
        iv.contentMode = .scaleAspectFit
        iv.tintColor = UIColor.blackPrimaryText()
        iv.translatesAutoresizingMaskIntoConstraints = false
        return iv
    }()

    private let titleLabel: UILabel = {
        let label = UILabel()
        label.font = UIFont.systemFont(ofSize: 15, weight: .bold)
        label.textColor = UIColor.blackPrimaryText()
        label.translatesAutoresizingMaskIntoConstraints = false
        return label
    }()

    private let subtitleLabel: UILabel = {
        let label = UILabel()
        label.font = UIFont.systemFont(ofSize: 15)
        label.textColor = UIColor.blackSecondaryText()
        label.numberOfLines = 2
        label.translatesAutoresizingMaskIntoConstraints = false
        return label
    }()

    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        setupViews()
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    private func setupViews() {
        contentView.addSubview(iconView)
        contentView.addSubview(titleLabel)
        contentView.addSubview(subtitleLabel)
        
        NSLayoutConstraint.activate([
            iconView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 12),
            iconView.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
            iconView.widthAnchor.constraint(equalToConstant: 36),
            iconView.heightAnchor.constraint(equalToConstant: 36),
            
            titleLabel.leadingAnchor.constraint(equalTo: iconView.trailingAnchor, constant: 12),
            titleLabel.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -12),
            titleLabel.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 8),
            
            subtitleLabel.leadingAnchor.constraint(equalTo: titleLabel.leadingAnchor),
            subtitleLabel.trailingAnchor.constraint(equalTo: titleLabel.trailingAnchor),
            subtitleLabel.topAnchor.constraint(equalTo: titleLabel.bottomAnchor, constant: 2),
            subtitleLabel.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -8)
        ])
    }
    
    func configure(with step: MWMRouteStepInfo) {
        titleLabel.text = step.formattedDistance
        subtitleLabel.text = step.textualInstruction
        iconView.image = turnImage(for: step)
    }
    
    private func turnImage(for step: MWMRouteStepInfo) -> UIImage? {
        let carDir = step.carDirection
        let pedDir = step.pedestrianDirection
        
        var imageName: String?
        
        switch carDir {
            case 1: imageName = "ic_cp_straight"
            case 2: imageName = "ic_cp_simple_right"
            case 3: imageName = "ic_cp_sharp_right"
            case 4: imageName = "ic_cp_slight_right"
            case 5: imageName = "ic_cp_simple_left"
            case 6: imageName = "ic_cp_sharp_left"
            case 7: imageName = "ic_cp_slight_left"
            case 8: imageName = "ic_cp_uturn_left"
            case 9: imageName = "ic_cp_uturn_right"
            case 10, 11, 12: imageName = "ic_cp_round"
            case 14: imageName = "ic_cp_finish_point"
            case 15: imageName = "ic_cp_exit_highway_to_left"
            case 16: imageName = "ic_cp_exit_highway_to_right"
            default: break
        }
        
        if imageName == nil && pedDir > 0 {
            switch pedDir {
                case 1: imageName = "ic_cp_straight"
                case 2: imageName = "ic_cp_simple_right"
                case 3: imageName = "ic_cp_simple_left"
                case 4: imageName = "ic_cp_finish_point"
                default: break
            }
        }
        
        guard let name = imageName else { return nil }
        return UIImage(named: name)?.withRenderingMode(.alwaysTemplate)
    }
}
