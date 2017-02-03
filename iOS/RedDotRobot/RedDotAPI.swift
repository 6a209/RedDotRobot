//
//  RedDotAPI.swift
//  RedDotRobot
//
//  Created by Blank on 2017/1/25.
//  Copyright © 2017年 Blank. All rights reserved.
//

import UIKit
import Foundation

class RedDotAPI: NSObject {
    
    private static let sharedInstance = RedDotAPI()
    
    private lazy var urlSession: URLSession = URLSession.shared
    
    public static var url: String?
    
    func setMode(mode: Int) -> () {
        
        guard let urlString = RedDotAPI.url?.appending("/mode?mode=\(mode)") else {
            return
        }
        
        guard let url = URL(string: urlString) else {
            return
        }
        
        let task = urlSession.dataTask(with: url) { (data, urlResponse, error) in
            if error != nil {
                print(error!);
            }
        }
        
        task.resume()
        
    }
}
